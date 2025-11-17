#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <boost/json.hpp>
#include <concepts>
#include <ranges>
#include <string_view>
#include <optional>

namespace net = boost::asio;
namespace http = boost::beast::http;
namespace url = boost::urls;
namespace beast = boost::beast;
namespace json = boost::json;

using tcp = net::ip::tcp;

namespace wolf {

    namespace client {
        namespace detail {
            // Parse and validate URL
            inline url::url parse_url(std::string_view url_str) {
                boost::system::result<url::url> result = url::parse_uri(url_str);
                if (!result) {
                    throw std::invalid_argument("Invalid URL");
                }
                return *result;
            }

            // Establish connection to host
            inline beast::tcp_stream connect_to_host(const url::url& url, int port) {
                boost::system::error_code ec;
                net::io_context ioc;
                
                tcp::resolver resolver(ioc);
                auto const results = resolver.resolve(url.host(), std::to_string(port), ec);
                if (ec) {
                    throw boost::system::system_error(ec);
                }

                beast::tcp_stream stream(ioc);
                stream.connect(results, ec);
                if (ec) {
                    throw boost::system::system_error(ec);
                }
                
                return stream;
            }

            // Send HTTP request and receive response
            inline http::response<http::string_body> send_and_receive(
                beast::tcp_stream& stream,
                http::request<http::string_body>& req
            ) {
                boost::system::error_code ec;
                
                // Send the request
                http::write(stream, req, ec);
                if (ec) {
                    throw boost::system::system_error(ec);
                }

                // Receive the response
                beast::flat_buffer buffer;
                http::response<http::string_body> res;
                http::read(stream, buffer, res, ec);
                if (ec) {
                    throw boost::system::system_error(ec);
                }

                // Gracefully close the socket
                stream.socket().shutdown(tcp::socket::shutdown_both, ec);
                
                return res;
            }

            // Create HTTP request with common headers
            inline http::request<http::string_body> create_request(
                http::verb method,
                const url::url& url,
                const std::optional<std::string>& body = std::nullopt,
                const std::optional<std::string>& content_type = std::nullopt
            ) {
                http::request<http::string_body> req{method, url.encoded_target(), 11};
                req.set(http::field::host, url.host());
                req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
                
                if (body.has_value()) {
                    if (content_type.has_value()) {
                        req.set(http::field::content_type, *content_type);
                    }
                    req.body() = *body;
                    req.prepare_payload();
                }
                
                return req;
            }

            // Generic HTTP request handler
            inline http::response<http::string_body> execute_request(
                std::string_view url_str,
                http::verb method,
                int port = 80,
                const std::optional<std::string>& body = std::nullopt,
                const std::optional<std::string>& content_type = std::nullopt
            ) {
                auto url = parse_url(url_str);
                auto stream = connect_to_host(url, port);
                auto req = create_request(method, url, body, content_type);
                return send_and_receive(stream, req);
            }
        }

        inline http::response<http::string_body> get_request(std::string_view url_str, const int port = 80)
        {
            return detail::execute_request(url_str, http::verb::get, port);
        }

        inline http::response<http::string_body> post_request(std::string_view url_str, const std::string& body, const std::string& content_type = "application/json", const int port = 80)
        {
            return detail::execute_request(url_str, http::verb::post, port, body, content_type);
        }

        inline http::response<http::string_body> put_request(std::string_view url_str, const std::string& body, const std::string& content_type = "application/json", const int port = 80)
        {
            return detail::execute_request(url_str, http::verb::put, port, body, content_type);
        }

        inline http::response<http::string_body> delete_request(std::string_view url_str, const int port = 80)
        {
            return detail::execute_request(url_str, http::verb::delete_, port);
        }

        inline http::response<http::string_body> patch_request(std::string_view url_str, const std::string& body, const std::string& content_type = "application/json", const int port = 80)
        {
            return detail::execute_request(url_str, http::verb::patch, port, body, content_type);
        }
    }
}