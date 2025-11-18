#include "../src/wolf.hpp"
#include <iostream>
#include <chrono>
#include <boost/asio.hpp>

using namespace wolf;
namespace net = boost::asio;

int main() {
    // Create a single unified router that accepts BOTH sync and async handlers
    web_server server(8080);
    
    // ✅ Synchronous handler - returns http_response directly
    server->get("/sync", [](const http_request& req) {
        std::cout << "Handling /sync synchronously\n";
        return http_response(200).text("Sync response");
    });
    
    // ✅ Asynchronous handler - returns awaitable<http_response>
    server->get("/async", [](const http_request& req) -> net::awaitable<http_response> {
        std::cout << "Handling /async asynchronously\n";
        
        // Simple async response without timer
        co_return http_response(200).text("Async response");
    });
    
    // ✅ Another sync handler - with JSON response
    server->post("/data", [](const http_request& req) {
        std::cout << "Handling /data synchronously with JSON\n";
        boost::json::object result = {
            {"status", "success"},
            {"timestamp", std::time(nullptr)}
        };
        return http_response(200).json(result);
    });
    
    // ✅ Another async handler - with parameter extraction
    server->get("/users/:id", [](const http_request& req) -> net::awaitable<http_response> {
        std::cout << "Handling /users/:id asynchronously\n";
        
        auto user_id = req.params().at("id");
        boost::json::object user = {
            {"id", user_id},
            {"name", "John Doe"},
            {"email", "john@example.com"}
        };
        
        co_return http_response(200).json(user);
    });
    
    std::cout << "🚀 Server running on http://localhost:8080\n";
    std::cout << "\nAvailable endpoints:\n";
    std::cout << "  GET  /sync        - Synchronous handler\n";
    std::cout << "  GET  /async       - Asynchronous handler with delay\n";
    std::cout << "  POST /data        - Synchronous JSON response\n";
    std::cout << "  GET  /users/:id   - Asynchronous handler with parameter\n";
    std::cout << "\nTest with:\n";
    std::cout << "  curl http://localhost:8080/sync\n";
    std::cout << "  curl http://localhost:8080/async\n";
    std::cout << "  curl -X POST http://localhost:8080/data\n";
    std::cout << "  curl http://localhost:8080/users/123\n\n";
    
    server.start();
    
    return 0;
}
