#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <boost/json.hpp>
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

#include "http_request.hpp"
#include "http_response.hpp"
#include "http_router.hpp"

namespace net = boost::asio;
namespace http = boost::beast::http;
namespace url = boost::urls;
namespace beast = boost::beast;
namespace json = boost::json;

using tcp = net::ip::tcp;

namespace wolf {
    using callback_t = std::function<wolf::http_response(const wolf::http_request&)>;
    using async_callback_t = std::function<net::awaitable<wolf::http_response>(const wolf::http_request&)>;

    beast::string_view mime_type(beast::string_view path)
    {
        using beast::iequals;
        auto const ext = [&path]
        {
            auto const pos = path.rfind(".");
            if(pos == beast::string_view::npos)
                return beast::string_view{};
            return path.substr(pos);
        }();

        if(iequals(ext, ".htm"))  return "text/html";
        if(iequals(ext, ".html")) return "text/html";
        if(iequals(ext, ".php"))  return "text/html";
        if(iequals(ext, ".css"))  return "text/css";
        if(iequals(ext, ".txt"))  return "text/plain";
        if(iequals(ext, ".js"))   return "application/javascript";
        if(iequals(ext, ".json")) return "application/json";
        if(iequals(ext, ".xml"))  return "application/xml";
        if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
        if(iequals(ext, ".flv"))  return "video/x-flv";
        if(iequals(ext, ".png"))  return "image/png";
        if(iequals(ext, ".jpe"))  return "image/jpeg";
        if(iequals(ext, ".jpeg")) return "image/jpeg";
        if(iequals(ext, ".jpg"))  return "image/jpeg";
        if(iequals(ext, ".gif"))  return "image/gif";
        if(iequals(ext, ".bmp"))  return "image/bmp";
        if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
        if(iequals(ext, ".tiff")) return "image/tiff";
        if(iequals(ext, ".tif"))  return "image/tiff";
        if(iequals(ext, ".svg"))  return "image/svg+xml";
        if(iequals(ext, ".svgz")) return "image/svg+xml";

        return "application/text";
    }

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
                    else if(ec == net::error::timed_out) {
                        // Connection timed out, just exit
                        break;
                    }
                    else if(ec == net::error::connection_reset) {
                        // Connection reset by peer, just exit
                        break;
                    }
                    else if(ec) {
                        std::cerr << "Read error: " << ec.message() << std::endl;
                        std::cerr << "Error code: " << ec.value() << std::endl;
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

                    if (handler) {
                        // Get middleware for the route (exact + wildcard pattern matches)
                        using middleware_t = http_middleware<http_request, http_response>;
                        std::vector<middleware_t*> middlewares = router_.get_middlewares(std::string(target_clean));

                        // Populate request with parameters
                        wolf::http_request req(request_, params, query_params);

                        // Custom request info header for detect unique visitors
                        req.set("Client-Address", socket_.remote_endpoint().address().to_string());
                        
                        // Use C++20 ranges for parameter setting
                        for (const auto& [key, value] : params) {
                            req.set(key, value);
                        }

                        if(!middlewares.empty()) {
                            // Apply middleware before handler
                            for(middleware_t* middleware : middlewares) {
                                if(!middleware->before_request(req)) {
                                    // Middleware blocked the request
                                    response = middleware->blocked_response(req);

                                    auto self = this->shared_from_this();
                                    co_await beast::http::async_write(socket_, response, net::use_awaitable);
                                    
                                    co_return;
                                }
                            }
                        }
                        
                        // Router always returns awaitable now - unified handling
                        response = co_await handler(req);

                        if(!middlewares.empty()) {
                            // Apply middleware before sending response                            
                            std::for_each(middlewares.begin(), middlewares.end(),
                                [&req, &response](const auto& middleware) {
                                    middleware->after_response(req, response);
                                });
                        }
                    } 
                    else {
                        // check if file exists for static serving
                        beast::error_code file_ec;
                        beast::http::file_body::value_type file_body;

                        std::string file_target = std::string(target_clean.substr(1)); // remove leading '/'
                        bool is_valid_file_path = !file_target.empty() && (file_target.find("..") == beast::string_view::npos);

                        if(is_valid_file_path) {
                            auto file_path = std::string("static/") + std::string(file_target);
                            file_body.open(file_path.c_str(), beast::file_mode::scan, file_ec);

                            if(!file_ec) {
                                http::response<http::file_body> static_response{
                                        std::piecewise_construct,
                                        std::make_tuple(std::move(file_body)),
                                        std::make_tuple(http::status::ok, request_.version())};
                                
                                static_response.prepare_payload();
                                
                                auto self = this->shared_from_this();
                                co_await beast::http::async_write(socket_, static_response, net::use_awaitable);

                                co_return;
                            }
                        }

                        response.result(http::status::not_found);
                        response.body() = "404 Not Found";
                    }
                    
                    response.version(request_.version() > 0 ? request_.version() : 11);
                    response.set(http::field::server, "WolfServer/2.0");
                    response.prepare_payload();

                    auto self = this->shared_from_this();
                    co_await beast::http::async_write(socket_, response, net::use_awaitable);
                    
                    if(request_.need_eof()) {
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

            // Reference accessor for easier binding with .
            [[nodiscard]] wolf_router& router() noexcept { return router_; }
            [[nodiscard]] const wolf_router& router() const noexcept { return router_; }

            // C++20: Add explicit stop method
            void stop() noexcept {
                ioc_->stop();
            }
    };

}