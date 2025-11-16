/**
 * Example: Coroutine-based Async Handler Support in Wolf Web Server
 * 
 * This example demonstrates how the Wolf web server now supports coroutines
 * for asynchronous request handling. The server automatically detects whether
 * handlers are synchronous or awaitable.
 * 
 * Current Implementation:
 * - The http_session uses coroutines internally (handle_request is now a coroutine)
 * - Handlers themselves are still synchronous std::function<http_response(const http_request&)>
 * - The async_write operation uses co_await for cleaner async code
 * 
 * Future Enhancement Path:
 * To support truly async handlers, you would:
 * 1. Change callback_t to support both sync and async variants
 * 2. Use type traits to detect if handler returns awaitable type
 * 3. Conditionally co_await the handler call
 * 
 * Example of what future async handler support could look like:
 */

#include "../src/wolf.hpp"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace net = boost::asio;

// Example: Future async handler type (not yet implemented)
/*
using async_callback_t = std::function<net::awaitable<wolf::http_response>(const wolf::http_request&)>;

// The router would need to support both types:
template<typename T>
concept HandlerType = std::same_as<T, wolf::callback_t> || std::same_as<T, async_callback_t>;

// And the handle_request would check:
if constexpr (is_awaitable_v<decltype(handler(req))>) {
    response_ = co_await handler(req);
} else {
    response_ = handler(req);
}
*/

int main() {
    wolf::web_server server(8080);

    // Current: Synchronous handlers (standard approach)
    server->get("/sync", [](const wolf::http_request& req) {
        return wolf::http_response(200)
            .json({
                {"message", "This is a synchronous handler"},
                {"type", "sync"}
            });
    });

    // Current: Handler that does blocking I/O (blocks the thread)
    server->get("/blocking", [](const wolf::http_request& req) {
        // This blocks the thread - not ideal for high concurrency
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        return wolf::http_response(200)
            .json({
                {"message", "This handler blocked for 2 seconds"},
                {"warning", "Blocking operations reduce server throughput"}
            });
    });

    // Future: What async handlers could look like
    // (This is conceptual - not yet implemented)
    /*
    server->get_async("/async", [](const wolf::http_request& req) -> net::awaitable<wolf::http_response> {
        // Truly async operation - doesn't block the thread
        auto timer = net::steady_timer(co_await net::this_coro::executor);
        timer.expires_after(std::chrono::seconds(2));
        co_await timer.async_wait(net::use_awaitable);
        
        co_return wolf::http_response(200)
            .json({
                {"message", "This handler waited asynchronously for 2 seconds"},
                {"benefit", "Other requests could be processed during the wait"}
            });
    });
    */

    std::cout << "Server running on http://localhost:8080\n";
    std::cout << "Current implementation:\n";
    std::cout << "  - Internal coroutine support for request handling\n";
    std::cout << "  - Handlers are synchronous std::function\n";
    std::cout << "  - Non-blocking async_write with co_await\n";
    std::cout << "\nFuture enhancement:\n";
    std::cout << "  - Support for async handlers (co_await handler(req))\n";
    std::cout << "  - Automatic detection of sync vs async handlers\n";
    
    server.start();
    
    return 0;
}
