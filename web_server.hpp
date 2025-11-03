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
        beast::websocket::stream<beast::tcp_stream> ws_;
        public:
            websocket_session(tcp::socket socket)
                : ws_(std::move(socket)) {}

            void start() {
                ws_.async_accept(
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

                // Echo the message back
                ws_.text(ws_.got_text());
                ws_.async_write(
                    buffer_.data(),
                    beast::bind_front_handler(
                        &websocket_session::on_write,
                        shared_from_this()));
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
                    // Create a websocket session and transfer control
                    std::make_shared<websocket_session>(std::move(socket_))->start();
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
                    case http::verb::delete_: method = DELETE; break;
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