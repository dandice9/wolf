#include "../src/wolf.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <iostream>

namespace net = boost::asio;
using wolf::http_request;
using wolf::http_response;
using wolf::response_t;

// Example 1: Synchronous handler (existing functionality)
response_t sync_handler(const http_request& req) {
    response_t res;
    res.result(boost::beast::http::status::ok);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.body() = "Sync response";
    res.prepare_payload();
    return res;
}

// Example 2: Async handler returning awaitable<response_t>
net::awaitable<response_t> async_handler(const http_request& req) {
    // Simulate async work (e.g., database query, external API call)
    net::steady_timer timer(co_await net::this_coro::executor);
    timer.expires_after(std::chrono::milliseconds(100));
    co_await timer.async_wait(net::use_awaitable);
    
    response_t res;
    res.result(boost::beast::http::status::ok);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.body() = "Async response after 100ms";
    res.prepare_payload();
    co_return res;
}

// Example 3: Async handler returning awaitable<string>
net::awaitable<std::string> async_string_handler(const http_request& req) {
    // Simulate async work
    net::steady_timer timer(co_await net::this_coro::executor);
    timer.expires_after(std::chrono::milliseconds(50));
    co_await timer.async_wait(net::use_awaitable);
    
    co_return "Async string response";
}

// Example 4: Async handler with path parameters
net::awaitable<response_t> async_user_handler(const http_request& req) {
    // Simulate database lookup
    net::steady_timer timer(co_await net::this_coro::executor);
    timer.expires_after(std::chrono::milliseconds(75));
    co_await timer.async_wait(net::use_awaitable);
    
    auto user_id = req.uri_params().at("id");
    
    response_t res;
    res.result(boost::beast::http::status::ok);
    res.set(boost::beast::http::field::content_type, "application/json");
    res.body() = std::format(R"({{"user_id": "{}", "name": "User {}"}})", user_id, user_id);
    res.prepare_payload();
    co_return res;
}

// Example 5: Async handler that might throw
net::awaitable<response_t> async_error_handler(const http_request& req) {
    net::steady_timer timer(co_await net::this_coro::executor);
    timer.expires_after(std::chrono::milliseconds(10));
    co_await timer.async_wait(net::use_awaitable);
    
    // Simulate error condition
    if (req.target().find("error") != std::string::npos) {
        throw std::runtime_error("Simulated async error");
    }
    
    response_t res;
    res.result(boost::beast::http::status::ok);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.body() = "Success";
    res.prepare_payload();
    co_return res;
}

int main() {
    try {
        net::io_context ioc;
        
        // Create router with both sync and async handlers
        wolf::wolf_router router;
        
        // Register synchronous handler
        router.get("/sync", sync_handler);
        
        // Register async handlers
        router.get("/async", async_handler);
        router.get("/async-string", async_string_handler);
        router.get("/users/:id", async_user_handler);
        router.get("/test-error", async_error_handler);
        
        // You can also use POST, PUT, DELETE, etc. with async handlers
        router.post("/async-post", [](const http_request& req) -> net::awaitable<response_t> {
            // Process POST data asynchronously
            net::steady_timer timer(co_await net::this_coro::executor);
            timer.expires_after(std::chrono::milliseconds(50));
            co_await timer.async_wait(net::use_awaitable);
            
            response_t res;
            res.result(boost::beast::http::status::created);
            res.set(boost::beast::http::field::content_type, "application/json");
            res.body() = R"({"status": "created", "async": true})";
            res.prepare_payload();
            co_return res;
        });
        
        // Create and start web server
        wolf::web_server server(ioc, router, 8080);
        
        std::cout << "Server running on http://localhost:8080\n";
        std::cout << "Try these endpoints:\n";
        std::cout << "  GET  /sync          - Synchronous handler\n";
        std::cout << "  GET  /async         - Async handler (100ms delay)\n";
        std::cout << "  GET  /async-string  - Async string handler (50ms delay)\n";
        std::cout << "  GET  /users/123     - Async handler with params (75ms delay)\n";
        std::cout << "  GET  /test-error    - Async error handling test\n";
        std::cout << "  POST /async-post    - Async POST handler\n";
        std::cout << "\nPress Ctrl+C to stop.\n\n";
        
        ioc.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
