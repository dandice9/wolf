#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <boost/json.hpp>
#include "http_router.hpp"

namespace net = boost::asio;
namespace http = boost::beast::http;
namespace url = boost::urls;
namespace beast = boost::beast;
namespace json = boost::json;

using tcp = net::ip::tcp;

namespace wolf {
    using request_t = beast::http::request<beast::http::string_body>;
    using response_t = beast::http::response<beast::http::string_body>;

    response_t make_response(
        const std::string& body,
        beast::http::status status = beast::http::status::ok,
        const std::string& content_type = "text/plain") 
    {
        response_t res{status, 11};
        res.set(beast::http::field::content_type, content_type);
        res.body() = body;
        
        return res;
    }

    response_t set_cookie(
        response_t& res,
        const std::string& key,
        const std::string& value,
        const std::string& path = "/",
        const std::string& domain = "",
        int max_age = -1,
        bool http_only = true,
        bool secure = false) 
    {
        std::string cookie = key + "=" + value + "; Path=" + path;
        if (!domain.empty()) {
            cookie += "; Domain=" + domain;
        }
        if (max_age >= 0) {
            cookie += "; Max-Age=" + std::to_string(max_age);
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
        boost::json::value json_body_;
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
                boost::system::error_code ec;
                json_body_ = json::parse(this->body(), ec);

                if(ec) {
                    // Handle parse error if necessary
                    json_body_ = nullptr; // or some default value
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

            auto cookies() const {
                auto it = this->find(http::field::cookie);
                params_t cookies;
                if(it != this->end()) {
                    std::string cookie_str = it->value();
                    std::vector<std::string> cookie_pairs;
                    size_t start = 0;
                    size_t end = 0;
                    while((end = cookie_str.find(';', start)) != std::string::npos) {
                        cookie_pairs.push_back(cookie_str.substr(start, end - start));
                        start = end + 1;
                    }
                    cookie_pairs.push_back(cookie_str.substr(start));

                    for(const auto& pair : cookie_pairs) {
                        auto eq_pos = pair.find('=');
                        if(eq_pos != std::string::npos) {
                            std::string key = pair.substr(0, eq_pos);
                            std::string value = pair.substr(eq_pos + 1);
                            // Trim whitespace
                            key.erase(0, key.find_first_not_of(" \t"));
                            key.erase(key.find_last_not_of(" \t") + 1);
                            value.erase(0, value.find_first_not_of(" \t"));
                            value.erase(value.find_last_not_of(" \t") + 1);
                            cookies[key] = value;
                        }
                    }
                }
                return cookies;
            }

            auto get(const std::string& key) const {
                auto it = uri_params_.find(key);
                if(it != uri_params_.end()) {
                    return it->second;
                }
                it = query_params_.find(key);
                if(it != query_params_.end()) {
                    return it->second;
                }
                return std::string{};
            }
    };

    using wolf_router = http_router<response_t, http_request>;

    class websocket_session : public std::enable_shared_from_this<websocket_session> {
        beast::flat_buffer buffer_;
        beast::websocket::stream<tcp::socket> ws_;
        std::function<std::string(const std::string&)> socket_handler_;
        
        public:
            // Constructor takes socket only
            websocket_session(tcp::socket socket, 
                              const std::function<std::string(const std::string&)>& handler = nullptr)
                : ws_(std::move(socket)), socket_handler_(handler)
            {
                // Set WebSocket options
                ws_.set_option(beast::websocket::stream_base::decorator(
                    [](beast::websocket::response_type& res) {
                        res.set(http::field::server, "WolfServer");
                    }));
            }

            // Start with already-read request
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
            void on_accept(beast::error_code ec) {
                if(ec)
                    return;

                do_read();
            }

            void on_write(beast::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if(ec)
                    return;

                buffer_.consume(buffer_.size());

                do_read();
            }

            void on_read(beast::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if(ec == beast::websocket::error::closed)
                    return;

                if(ec)
                    return;

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

            void on_close(beast::error_code ec) {
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

    class http_session : public std::enable_shared_from_this<http_session> {
        tcp::socket socket_;
        beast::flat_buffer buffer_;
        wolf_router& router_;
        request_t request_;
        response_t response_;

        public:
            http_session(tcp::socket socket, wolf_router& router)
                : socket_(std::move(socket)), router_(router) {}

            void start() {
                do_read();
            }

        private:
            void do_read() {
                auto self = shared_from_this();
                request_ = {};
                http::async_read(socket_, buffer_, request_,
                    [this, self](beast::error_code ec, std::size_t bytes_transferred) {
                        if (!ec) {
                            handle_request();
                        }
                    });
            }

            void handle_request() {
                // check if it's a websocket upgrade
                if (beast::websocket::is_upgrade(request_)) {
                    // Create a websocket session and pass the request for upgrade
                    auto ws_session = std::make_shared<websocket_session>(std::move(socket_), 
                                                                          router_.get_socket_handler());
                    ws_session->run(std::move(request_));

                    return;
                }

                std::string target = std::string(request_.target());
                // Remove query string if present
                auto pos = target.find('?');
                auto target_clean = (pos != std::string::npos) ? target.substr(0, pos) : target;
                
                // Convert Beast HTTP method to wolf http_method
                http_method method;
                switch(request_.method()) {
                    case http::verb::get: method = GET; break;
                    case http::verb::post: method = POST; break;
                    case http::verb::put: method = PUT; break;
                    case http::verb::delete_: method = DEL; break;
                    case http::verb::patch: method = PATCH; break;
                    case http::verb::options: method = OPTIONS; break;
                    case http::verb::head: method = HEAD; break;
                    case http::verb::connect: method = CONNECT; break;
                    case http::verb::trace: method = TRACE; break;
                    default: method = GET; break;
                }

                auto query_params = url::parse_query(target);

                auto [is_trie, handler, params] = router_.resolve(method, target_clean);

                // Use member variable for response
                response_ = {};

                if (handler) {
                    // Populate request with parameters
                    http_request req(request_, params, query_params);
                    for (const auto& [key, value] : params) {
                        req.set(key, value);
                    }
                    
                    response_ = handler(req);
                } else {
                    response_.result(http::status::not_found);
                    response_.body() = "404 Not Found";
                }
                response_.version(request_.version());
                response_.set(http::field::server, "WolfServer");
                response_.prepare_payload();

                auto self = shared_from_this();
                http::async_write(socket_, response_,
                    [this, self](beast::error_code ec, std::size_t) {
                        if(ec || request_.need_eof()) {
                            socket_.shutdown(tcp::socket::shutdown_send, ec);
                            return;
                        }
                        
                        do_read();
                    });
            }
    };

    class web_server {
        std::vector<std::thread> threads_;
        std::unique_ptr<net::io_context> ioc_;
        std::unique_ptr<tcp::acceptor> acceptor_;
        wolf_router router_;

        void do_accept() {
            acceptor_->async_accept(
                [this](beast::error_code ec, tcp::socket socket) {
                    if (!ec) {
                        std::make_shared<http_session>(std::move(socket), router_)->start();
                    }
                    
                    // Accept next connection
                    do_accept();
                });
        }

        public:
            web_server(unsigned short port = 8080){
                // Initialize server components
                const auto thread_count = std::thread::hardware_concurrency();
                ioc_ = std::make_unique<net::io_context>(thread_count);
                
                // Create acceptor
                acceptor_ = std::make_unique<tcp::acceptor>(
                    *ioc_,
                    tcp::endpoint(tcp::v4(), port)
                );
                
                // Start accepting connections
                do_accept();

                // Start worker threads
                if(thread_count > 1) {
                    for(unsigned int i = 0; i < thread_count-1; ++i) {
                        threads_.emplace_back([this]() {
                            ioc_->run();
                        });
                    }
                }
            }

            ~web_server() {
                ioc_->stop();
                for(auto& thread : threads_) {
                    if(thread.joinable()) {
                        thread.join();
                    }
                }
            }

            void start(){
                ioc_->run();
            }

            wolf_router* operator->(){ return &router_; }
            const wolf_router* operator->() const { return &router_; }
    };

}