#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <boost/json.hpp>
#include "http_router.hpp"
#include <concepts>
#include <ranges>
#include <string_view>
#include <format>
#include <source_location>
#include <expected>
#include <functional>
#include <coroutine>
#include <type_traits>
#include <iostream>

namespace net = boost::asio;
namespace http = boost::beast::http;
namespace url = boost::urls;
namespace beast = boost::beast;
namespace json = boost::json;

using tcp = net::ip::tcp;

namespace wolf {
    class http_request;
    class http_response;

    using request_t = beast::http::request<beast::http::string_body>;
    using response_t = beast::http::response<beast::http::string_body>;
    using callback_t = std::function<wolf::http_response(const wolf::http_request&)>;
    using async_callback_t = std::function<net::awaitable<wolf::http_response>(const wolf::http_request&)>;

    // C++20 concepts for type safety
    template<typename T>
    concept StringLike = std::convertible_to<T, std::string_view>;

    template<typename T>
    concept StatusType = std::same_as<T, beast::http::status>;

    // C++20 [[nodiscard]] and constexpr where possible
    [[nodiscard]] inline response_t make_response(
        std::string_view body,
        beast::http::status status = beast::http::status::ok,
        std::string_view content_type = "text/plain") 
    {
        response_t res{status, 11};
        res.set(beast::http::field::content_type, content_type);
        res.body() = body;
        res.prepare_payload();
        
        return res;
    }

    // C++20 std::format for cleaner string building
    template<StringLike K, StringLike V>
    response_t set_cookie(
        response_t& res,
        K&& key,
        V&& value,
        std::string_view path = "/",
        std::string_view domain = "",
        int max_age = -1,
        bool http_only = true,
        bool secure = false) 
    {
        // Use std::format (C++20) for cleaner cookie string construction
        std::string cookie = std::format("{}={}; Path={}", 
                                        std::forward<K>(key), 
                                        std::forward<V>(value), 
                                        path);
        
        if (!domain.empty()) {
            cookie += std::format("; Domain={}", domain);
        }
        if (max_age >= 0) {
            cookie += std::format("; Max-Age={}", max_age);
        }
        if (http_only) {
            cookie += "; HttpOnly";
        }
        if (secure) {
            cookie += "; Secure";
        }
        
        res.set(beast::http::field::set_cookie, cookie);
        return res;
    }

    class http_request : public request_t {
        params_t query_params_;
        params_t uri_params_;
        boost::json::object json_body_;
        public:
            http_request(const request_t& req,
                         const params_t& uri_params,
                         const decltype(url::parse_query(std::string()))& query_params)
                : request_t(req), uri_params_(uri_params)
            {
                if(query_params) {  
                    for(const auto v : *query_params) {
                        query_params_[std::string(v.key)] = std::string(v.value);
                    }
                }
                
                // Parse JSON body safely
                boost::system::error_code ec;
                auto parsed = json::parse(this->body(), ec);
                
                if(!ec && parsed.is_object()) {
                    json_body_ = parsed.as_object();
                } else {
                    // Handle parse error or non-object body
                    json_body_ = {};
                }
            }

            auto get_json_body() const {
                return json_body_;
            }

            auto params() const {
                return uri_params_;
            }

            auto query_params() const {
                return query_params_;
            }

            // C++20 ranges for cleaner cookie parsing
            [[nodiscard]] auto cookies() const {
                params_t cookies;
                
                if (auto it = this->find(http::field::cookie); it != this->end()) {
                    std::string_view cookie_str = it->value();
                    
                    // Use C++20 ranges to split and process cookies
                    auto cookie_pairs = cookie_str 
                        | std::views::split(';')
                        | std::views::transform([](auto&& rng) {
                            return std::string_view(&*rng.begin(), std::ranges::distance(rng));
                        });
                    
                    for (auto pair_view : cookie_pairs) {
                        if (auto eq_pos = pair_view.find('='); eq_pos != std::string_view::npos) {
                            auto key = pair_view.substr(0, eq_pos);
                            auto value = pair_view.substr(eq_pos + 1);
                            
                            // Trim whitespace using C++20 ranges
                            auto trim = [](std::string_view sv) -> std::string {
                                auto start = std::ranges::find_if_not(sv, ::isspace);
                                auto end = std::ranges::find_if_not(sv | std::views::reverse, ::isspace).base();
                                return std::string(start, end);
                            };
                            
                            cookies[trim(key)] = trim(value);
                        }
                    }
                }
                
                return cookies;
            }

            [[nodiscard]] auto headers() const {
                params_t headers;
                for (const auto& field : this->base()) {
                    headers[std::string(field.name_string())] = std::string(field.value());
                }
                return headers;
            }

            auto find_query_param(std::string_view key) const -> std::optional<std::string> {
                if (auto it = query_params_.find(std::string(key)); it != query_params_.end()) {
                    return it->second;
                }
                return std::nullopt;
            }

            auto find_uri_param(std::string_view key) const -> std::optional<std::string> {
                if (auto it = uri_params_.find(std::string(key)); it != uri_params_.end()) {
                    return it->second;
                }
                return std::nullopt;
            }

            auto find_header(std::string_view key) const -> std::optional<std::string> {
                if(auto it = this->find(key); it != this->end()) {
                    return std::string(it->value());
                }
                return std::nullopt;
            }

            auto find_cookie(std::string_view key) const -> std::optional<std::string> {
                auto cookies = this->cookies();
                if(auto it = cookies.find(std::string(key)); it != cookies.end()) {
                    return it->second;
                }
                return std::nullopt;
            }

            // C++20 std::optional for cleaner return semantics
            [[nodiscard]] std::optional<std::string> get(std::string_view key) const noexcept {
                if (auto it = uri_params_.find(std::string(key)); it != uri_params_.end()) {
                    return it->second;
                }
                if (auto it = query_params_.find(std::string(key)); it != query_params_.end()) {
                    return it->second;
                }
                if(auto it = this->cookies().find(std::string(key)); it != this->cookies().end()) {
                    return it->second;
                }
                if(auto it = this->find(key); it != this->end()) {
                    return std::string(it->value());
                }
                
                return std::nullopt;
            }

            // Convenience method that returns empty string if not found
            [[nodiscard]] std::string get_or(std::string_view key, std::string_view default_val = "") const noexcept {
                return get(key).value_or(std::string(default_val));
            }
    };

    class http_response : public response_t {
        public:
            // Default constructor
            http_response() : response_t(beast::http::status::ok, 11) {
                this->set(beast::http::field::server, "WolfServer/2.0");
            }

            // Constructor with status code (int or beast::http::status)
            explicit http_response(int status_code) 
                : response_t(static_cast<beast::http::status>(status_code), 11) {
                this->set(beast::http::field::server, "WolfServer/2.0");
            }

            explicit http_response(beast::http::status status) 
                : response_t(status, 11) {
                this->set(beast::http::field::server, "WolfServer/2.0");
            }

            // Allow base class constructors
            using response_t::response_t;

            // C++20 concepts for type safety
            template<StatusType S>
            http_response& set_status(S status) & {
                this->result(status);
                return *this;
            }

            template<StatusType S>
            http_response set_status(S status) && {
                this->result(status);
                return std::move(*this);
            }

            // Fluent API: Set JSON body with automatic content-type
            // Lvalue reference version - returns reference for chaining
            [[nodiscard]] http_response& json(const boost::json::value& json_value) & {
                this->set(beast::http::field::content_type, "application/json");
                this->body() = boost::json::serialize(json_value);
                this->prepare_payload();
                return *this;
            }

            // Rvalue reference version - returns by value to avoid dangling reference
            [[nodiscard]] http_response json(const boost::json::value& json_value) && {
                this->set(beast::http::field::content_type, "application/json");
                this->body() = boost::json::serialize(json_value);
                this->prepare_payload();
                return std::move(*this);
            }

            // Fluent API: Set JSON body from object (convertible to json::value)
            [[nodiscard]] http_response& json(const boost::json::object& json_obj) & {
                return json(boost::json::value(json_obj));
            }

            [[nodiscard]] http_response json(const boost::json::object& json_obj) && {
                return std::move(*this).json(boost::json::value(json_obj));
            }

            // Fluent API: Set JSON body from array
            [[nodiscard]] http_response& json(const boost::json::array& json_arr) & {
                return json(boost::json::value(json_arr));
            }

            [[nodiscard]] http_response json(const boost::json::array& json_arr) && {
                return std::move(*this).json(boost::json::value(json_arr));
            }

            // Fluent API: Set plain text body
            [[nodiscard]] http_response& text(std::string_view body_text) & {
                this->set(beast::http::field::content_type, "text/plain");
                this->body() = body_text;
                this->prepare_payload();
                return *this;
            }

            [[nodiscard]] http_response text(std::string_view body_text) && {
                this->set(beast::http::field::content_type, "text/plain");
                this->body() = body_text;
                this->prepare_payload();
                return std::move(*this);
            }

            // Fluent API: Set HTML body
            [[nodiscard]] http_response& html(std::string_view html_content) & {
                this->set(beast::http::field::content_type, "text/html");
                this->body() = html_content;
                this->prepare_payload();
                return *this;
            }

            [[nodiscard]] http_response html(std::string_view html_content) && {
                this->set(beast::http::field::content_type, "text/html");
                this->body() = html_content;
                this->prepare_payload();
                return std::move(*this);
            }

            // Fluent API: Set custom header
            template<StringLike K, StringLike V>
            [[nodiscard]] http_response& header(K&& key, V&& value) & {
                this->set(std::forward<K>(key), std::forward<V>(value));
                return *this;
            }

            template<StringLike K, StringLike V>
            [[nodiscard]] http_response header(K&& key, V&& value) && {
                this->set(std::forward<K>(key), std::forward<V>(value));
                return std::move(*this);
            }

            // Fluent API: Set cookie (convenience wrapper)
            template<StringLike K, StringLike V>
            [[nodiscard]] http_response& cookie(
                K&& key,
                V&& value,
                std::string_view path = "/",
                std::string_view domain = "",
                int max_age = -1,
                bool http_only = true,
                bool secure = false) &
            {
                set_cookie(*this, std::forward<K>(key), std::forward<V>(value), 
                          path, domain, max_age, http_only, secure);
                return *this;
            }

            template<StringLike K, StringLike V>
            [[nodiscard]] http_response cookie(
                K&& key,
                V&& value,
                std::string_view path = "/",
                std::string_view domain = "",
                int max_age = -1,
                bool http_only = true,
                bool secure = false) &&
            {
                set_cookie(*this, std::forward<K>(key), std::forward<V>(value), 
                          path, domain, max_age, http_only, secure);
                return std::move(*this);
            }

            // Fluent API: Send file (set content-disposition)
            [[nodiscard]] http_response& send_file(std::string_view filename, 
                                                   std::string_view content_type = "application/octet-stream") & {
                this->set(beast::http::field::content_type, content_type);
                this->set(beast::http::field::content_disposition, 
                         std::format("attachment; filename=\"{}\"", filename));
                this->prepare_payload();
                return *this;
            }

            [[nodiscard]] http_response send_file(std::string_view filename, 
                                                  std::string_view content_type = "application/octet-stream") && {
                this->set(beast::http::field::content_type, content_type);
                this->set(beast::http::field::content_disposition, 
                         std::format("attachment; filename=\"{}\"", filename));
                this->prepare_payload();
                return std::move(*this);
            }

            // Fluent API: Set status and return reference for chaining
            [[nodiscard]] http_response& status(int status_code) & {
                this->result(static_cast<beast::http::status>(status_code));
                return *this;
            }

            [[nodiscard]] http_response status(int status_code) && {
                this->result(static_cast<beast::http::status>(status_code));
                return std::move(*this);
            }

            [[nodiscard]] http_response& status(beast::http::status status_val) & {
                this->result(status_val);
                return *this;
            }

            [[nodiscard]] http_response status(beast::http::status status_val) && {
                this->result(status_val);
                return std::move(*this);
            }
    };

    // Unified router that automatically handles both sync and async handlers
    // No need to choose upfront - just use wolf_router!
    using wolf_router = http_router<net::awaitable<http_response>, http_request>;

    class websocket_session : public std::enable_shared_from_this<websocket_session> {
        beast::flat_buffer buffer_;
        beast::websocket::stream<tcp::socket> ws_;
        std::function<std::string(const std::string&)> socket_handler_;
        
        public:
            // Constructor takes socket only
            public:
            // C++20: Use explicit constructor with std::optional for handler
            explicit websocket_session(tcp::socket socket, 
                                       const std::function<std::string(const std::string&)>& handler = nullptr)
                : ws_(std::move(socket)), socket_handler_(handler)
            {
                // Set WebSocket options with C++20 lambda
                ws_.set_option(beast::websocket::stream_base::decorator(
                    [](beast::websocket::response_type& res) {
                        res.set(http::field::server, "WolfServer/2.0");
                    }));
            }

            // C++20: Use concepts for template constraint
            template<class Body, class Allocator>
            void run(http::request<Body, http::basic_fields<Allocator>> req) {
                // Accept the WebSocket upgrade using the parsed request
                ws_.async_accept(
                    req,
                    beast::bind_front_handler(
                        &websocket_session::on_accept,
                        shared_from_this()));
            }

        private:
            void on_accept(beast::error_code ec) noexcept {
                if(ec)
                    return;

                do_read();
            }

            void on_write(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) noexcept {
                if(ec)
                    return;

                buffer_.consume(buffer_.size());
                do_read();
            }

            void on_read(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) noexcept {
                if(ec == beast::websocket::error::closed)
                    return;

                if(ec)
                    return;

                // C++20: Use auto for type deduction
                const auto received_message = beast::buffers_to_string(buffer_.data());
                
                // If a socket handler is set, process the message
                if(socket_handler_) {
                    const auto response_message = socket_handler_(received_message);
                    ws_.text(ws_.got_text());
                    ws_.async_write(
                        net::buffer(response_message),
                        beast::bind_front_handler(
                            &websocket_session::on_write,
                            shared_from_this()));
                }
                else {
                    // close the connection if no handler is set
                    ws_.async_close(beast::websocket::close_code::normal,
                        beast::bind_front_handler(
                            &websocket_session::on_close,
                            shared_from_this()));
                }
            }

            void on_close(beast::error_code ec) noexcept {
                if(ec)
                    return;
            }

            void do_read() {
                ws_.async_read(buffer_,
                    beast::bind_front_handler(
                        &websocket_session::on_read,
                        shared_from_this()));
            }
    };

    template<typename Router>
    class http_session : public std::enable_shared_from_this<http_session<Router>> {
        tcp::socket socket_;
        beast::flat_buffer buffer_;
        Router& router_;
        wolf::response_t response_;

        public:
            http_session(tcp::socket socket, Router& router)
                : socket_(std::move(socket)), router_(router) {}

            void start() {
                net::co_spawn(
                    socket_.get_executor(),
                    handle_request(this->shared_from_this()),
                    net::detached
                );
            }

        private:
            net::awaitable<void> handle_request(const std::shared_ptr<http_session> self) {
                while (true)
                {
                    request_t request_;
                    beast::error_code ec;

                    const auto read_bytes = co_await beast::http::async_read(socket_, buffer_, request_, net::redirect_error(net::use_awaitable, ec));

                    if(ec == beast::http::error::end_of_stream) {
                        // Gracefully close the socket
                        socket_.shutdown(tcp::socket::shutdown_send, ec);
                        break;
                    }
                    else if(ec) {
                        std::cerr << "Read error: " << ec.message() << std::endl;
                        break;
                    }

                    // check if it's a websocket upgrade
                    if (beast::websocket::is_upgrade(request_)) {
                        // Create a websocket session and pass the request for upgrade
                        auto ws_session = std::make_shared<wolf::websocket_session>(std::move(socket_), 
                                                                                    router_.get_socket_handler());
                        ws_session->run(std::move(request_));
                        
                        break;
                    }

                    // Use std::string_view for efficiency (C++20)
                    std::string_view target = request_.target();
                    
                    // Remove query string if present
                    auto pos = target.find('?');
                    auto target_clean = (pos != std::string_view::npos) ? target.substr(0, pos) : target;
                    
                    // Convert Beast HTTP method to wolf http_method using constexpr map
                    wolf::http_method method = [&]() constexpr -> wolf::http_method {
                        switch(request_.method()) {
                            case http::verb::get: return wolf::http_method::GET;
                            case http::verb::post: return wolf::http_method::POST;
                            case http::verb::put: return wolf::http_method::PUT;
                            case http::verb::delete_: return wolf::http_method::DEL;
                            case http::verb::patch: return wolf::http_method::PATCH;
                            case http::verb::options: return wolf::http_method::OPTIONS;
                            case http::verb::head: return wolf::http_method::HEAD;
                            case http::verb::connect: return wolf::http_method::CONNECT;
                            case http::verb::trace: return wolf::http_method::TRACE;
                            default: return wolf::http_method::GET;
                        }
                    }();

                    auto query_params = url::parse_query(std::string(target));

                    // Use C++20 structured bindings
                    auto [is_trie, handler, params] = router_.resolve(method, target_clean);

                    // Use local http_response to avoid slicing and memory corruption
                    wolf::http_response response;

                    std::cout << "DEBUG: handler=" << (handler ? "found" : "null") << " target=" << target_clean << std::endl;

                    if (handler) {
                        // Populate request with parameters
                        wolf::http_request req(request_, params, query_params);
                        
                        // Use C++20 ranges for parameter setting
                        for (const auto& [key, value] : params) {
                            req.set(key, value);
                        }
                        
                        std::cout << "DEBUG: calling handler..." << std::endl;
                        // Router always returns awaitable now - unified handling
                        response = co_await handler(req);
                        std::cout << "DEBUG: handler returned, body size=" << response.body().size() << std::endl;
                    } else {
                        response.result(http::status::not_found);
                        response.body() = "404 Not Found";
                    }
                    
                    response.version(request_.version() > 0 ? request_.version() : 11);
                    response.set(http::field::server, "WolfServer/2.0");
                    response.prepare_payload();

                    std::cout << "DEBUG: writing response..." << std::endl;
                    auto self = this->shared_from_this();
                    co_await beast::http::async_write(socket_, response, net::use_awaitable);
                    std::cout << "DEBUG: response written" << std::endl;
                    
                    if(request_.need_eof()) {
                        beast::error_code ec;
                        socket_.shutdown(tcp::socket::shutdown_send, ec);
                        break;
                    }
                }
                
                co_return;
            }
    };

    class web_server {
        std::vector<std::thread> threads_;
        std::shared_ptr<net::io_context> ioc_;
        std::unique_ptr<tcp::acceptor> acceptor_;
        wolf_router router_;

        void do_accept() {
            acceptor_->async_accept(
                [this](beast::error_code ec, tcp::socket socket) {
                    if (!ec) {
                        std::make_shared<http_session<wolf_router>>(std::move(socket), router_)->start();
                    }
                    
                    // Accept next connection
                    do_accept();
                });
        }

        public:
            auto context() {
                return ioc_;
            }

            // C++20: Use explicit constructor with default port
            explicit web_server(unsigned short port = 8080) {
                // Initialize server components (C++20: use auto for clarity)
                const auto thread_count = std::thread::hardware_concurrency();
                ioc_ = std::make_unique<net::io_context>(thread_count);
                
                // Create acceptor
                acceptor_ = std::make_unique<tcp::acceptor>(
                    *ioc_,
                    tcp::endpoint(tcp::v4(), port)
                );
                
                // Start accepting connections
                do_accept();

                // C++20: Use ranges-based approach for worker threads
                if(thread_count > 1) {
                    threads_.reserve(thread_count - 1);
                    for(unsigned int i = 0; i < thread_count - 1; ++i) {
                        threads_.emplace_back([this]() {
                            ioc_->run();
                        });
                    }
                }
            }

            // C++20: Use noexcept for destructor
            ~web_server() noexcept {
                ioc_->stop();
                
                // C++20: Use ranges for cleaner iteration
                for(auto& thread : threads_) {
                    if(thread.joinable()) {
                        thread.join();
                    }
                }
            }

            // C++20: Add [[nodiscard]] for better API usage
            void start() {
                ioc_->run();
            }

            // C++20: Use noexcept for accessors
            [[nodiscard]] wolf_router* operator->() noexcept { return &router_; }
            [[nodiscard]] const wolf_router* operator->() const noexcept { return &router_; }

            // C++20: Add explicit stop method
            void stop() noexcept {
                ioc_->stop();
            }
    };

}