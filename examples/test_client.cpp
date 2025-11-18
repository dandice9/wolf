#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class http_client {
    net::io_context ioc_;
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;

public:
    http_client() : resolver_(ioc_), stream_(ioc_) {}

    std::pair<int, std::string> send_request(
        const std::string& method,
        const std::string& host,
        const std::string& port,
        const std::string& target,
        const std::string& body = "",
        const std::string& content_type = "application/json") {
        
        try {
            // Resolve the host
            auto const results = resolver_.resolve(host, port);

            // Connect to the server
            stream_.connect(results);

            // Set up the HTTP request
            if (method == "GET") {
                req_.method(http::verb::get);
            } else if (method == "POST") {
                req_.method(http::verb::post);
            } else if (method == "PUT") {
                req_.method(http::verb::put);
            } else if (method == "DELETE") {
                req_.method(http::verb::delete_);
            } else if (method == "PATCH") {
                req_.method(http::verb::patch);
            } else if (method == "HEAD") {
                req_.method(http::verb::head);
            } else if (method == "OPTIONS") {
                req_.method(http::verb::options);
            }

            req_.target(target);
            req_.version(11);
            req_.set(http::field::host, host);
            req_.set(http::field::user_agent, "WolfClient/1.0");
            
            if (!body.empty()) {
                req_.body() = body;
                req_.set(http::field::content_type, content_type);
                req_.prepare_payload();
            }

            // Send the HTTP request
            http::write(stream_, req_);

            // Read the HTTP response
            buffer_.clear();
            http::read(stream_, buffer_, res_);

            // Get response
            int status_code = static_cast<int>(res_.result_int());
            std::string response_body = res_.body();

            // Gracefully close the socket
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            return {status_code, response_body};

        } catch (std::exception const& e) {
            return {-1, std::string("Error: ") + e.what()};
        }
    }
};

void print_separator() {
    std::cout << std::string(70, '=') << std::endl;
}

void print_test_header(const std::string& test_name) {
    print_separator();
    std::cout << "TEST: " << test_name << std::endl;
    print_separator();
}

void print_result(const std::string& method, const std::string& endpoint, int status, const std::string& body) {
    std::cout << method << " " << endpoint << std::endl;
    std::cout << "Status: " << status << std::endl;
    std::cout << "Response:" << std::endl;
    std::cout << body << std::endl;
    std::cout << std::endl;
}

int main() {
    const std::string host = "localhost";
    const std::string port = "8080";

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           Wolf Web Server - HTTP Client Test Suite           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Testing server at: http://" << host << ":" << port << std::endl;
    std::cout << "\n";

    // Test 1: GET Root
    {
        print_test_header("GET / - Homepage");
        http_client client;
        auto [status, body] = client.send_request("GET", host, port, "/");
        print_result("GET", "/", status, body);
    }

    // Test 2: GET Health Check
    {
        print_test_header("GET /health - Health Check");
        http_client client;
        auto [status, body] = client.send_request("GET", host, port, "/health");
        print_result("GET", "/health", status, body);
    }

    // Test 3: GET All Users
    {
        print_test_header("GET /api/users - List All Users");
        http_client client;
        auto [status, body] = client.send_request("GET", host, port, "/api/users");
        print_result("GET", "/api/users", status, body);
    }

    // Test 4: GET User by ID (parameterized route)
    {
        print_test_header("GET /api/users/:id - Get User by ID");
        http_client client;
        auto [status, body] = client.send_request("GET", host, port, "/api/users/1");
        print_result("GET", "/api/users/1", status, body);
    }

    // Test 5: POST Create User
    {
        print_test_header("POST /api/users - Create New User");
        http_client client;
        std::string new_user = R"({"name": "David", "email": "david@example.com"})";
        auto [status, body] = client.send_request("POST", host, port, "/api/users", new_user);
        print_result("POST", "/api/users", status, body);
    }

    // Test 6: PUT Update User
    {
        print_test_header("PUT /api/users/:id - Update User");
        http_client client;
        std::string updated_user = R"({"name": "Alice Updated", "email": "alice.new@example.com"})";
        auto [status, body] = client.send_request("PUT", host, port, "/api/users/1", updated_user);
        print_result("PUT", "/api/users/1", status, body);
    }

    // Test 7: PATCH Partial Update User
    {
        print_test_header("PATCH /api/users/:id - Partial Update User");
        http_client client;
        std::string patch_data = R"({"email": "alice.patched@example.com"})";
        auto [status, body] = client.send_request("PATCH", host, port, "/api/users/1", patch_data);
        print_result("PATCH", "/api/users/1", status, body);
    }

    // Test 8: DELETE User
    {
        print_test_header("DELETE /api/users/:id - Delete User");
        http_client client;
        auto [status, body] = client.send_request("DELETE", host, port, "/api/users/1");
        print_result("DELETE", "/api/users/1", status, body);
    }

    // Test 9: GET Non-existent Route (404)
    {
        print_test_header("GET /nonexistent - Test 404");
        http_client client;
        auto [status, body] = client.send_request("GET", host, port, "/nonexistent");
        print_result("GET", "/nonexistent", status, body);
    }

    // Test 10: Multiple Parameterized Routes
    {
        print_test_header("GET /api/users/:userId - Multiple Parameters");
        http_client client;
        auto [status, body] = client.send_request("GET", host, port, "/api/users/123");
        print_result("GET", "/api/users/123", status, body);
    }

    print_separator();
    std::cout << "All tests completed!" << std::endl;
    print_separator();

    return 0;
}
