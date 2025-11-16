#pragma once
/**
 * Future Enhancement: Full Async Handler Support
 * 
 * This header shows how to extend Wolf to support both synchronous and
 * asynchronous handlers with automatic detection and handling.
 * 
 * To integrate this, the following changes would be needed:
 * 1. Update http_router.hpp to store variant handlers
 * 2. Update web_server.hpp to use conditional await
 * 3. Add registration methods for async handlers
 */

#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <variant>
#include <type_traits>

namespace wolf {
    namespace net = boost::asio;

    // Forward declarations
    class http_request;
    class http_response;

    // Type aliases
    using sync_callback_t = std::function<http_response(const http_request&)>;
    using async_callback_t = std::function<net::awaitable<http_response>(const http_request&)>;
    
    // Variant to hold either sync or async handler
    using handler_variant_t = std::variant<sync_callback_t, async_callback_t>;

    // Concept to check if a type is awaitable
    template<typename T>
    concept Awaitable = requires(T t) {
        { t.await_ready() } -> std::convertible_to<bool>;
        { t.await_suspend(std::coroutine_handle<>{}) };
        { t.await_resume() };
    };

    // Type trait to detect awaitable return types
    template<typename T>
    struct is_awaitable : std::false_type {};

    template<typename T>
    requires Awaitable<T>
    struct is_awaitable<T> : std::true_type {};

    template<typename T>
    inline constexpr bool is_awaitable_v = is_awaitable<T>::value;

    // Helper to invoke handler (sync or async)
    inline net::awaitable<http_response> invoke_handler(
        const handler_variant_t& handler,
        const http_request& req)
    {
        if (std::holds_alternative<async_callback_t>(handler)) {
            // Async handler - co_await it
            auto& async_handler = std::get<async_callback_t>(handler);
            co_return co_await async_handler(req);
        } else {
            // Sync handler - call directly
            auto& sync_handler = std::get<sync_callback_t>(handler);
            co_return sync_handler(req);
        }
    }

    /**
     * Example usage in http_session::handle_request():
     * 
     * net::awaitable<void> handle_request() {
     *     // ... existing code ...
     *     
     *     if (handler) {
     *         wolf::http_request req(request_, params, query_params);
     *         
     *         // Universal handler invocation (works for both sync and async)
     *         response_ = co_await invoke_handler(handler, req);
     *     } else {
     *         response_.result(http::status::not_found);
     *         response_.body() = "404 Not Found";
     *     }
     *     
     *     // ... rest of code ...
     * }
     */

    /**
     * Example router extension:
     * 
     * template <typename RT, typename PT>
     * class http_router {
     * private:
     *     boost::unordered_map<std::string, handler_variant_t> routes_;
     *     
     * public:
     *     // Add sync handler
     *     http_router& add(http_method method, std::string_view route, 
     *                      const sync_callback_t& handler) {
     *         auto key = std::format("{}:{}", method_to_string(method), route);
     *         routes_[key] = handler;
     *         return *this;
     *     }
     *     
     *     // Add async handler
     *     http_router& add_async(http_method method, std::string_view route,
     *                            const async_callback_t& handler) {
     *         auto key = std::format("{}:{}", method_to_string(method), route);
     *         routes_[key] = handler;
     *         return *this;
     *     }
     *     
     *     // Convenience methods
     *     http_router& get_async(std::string_view route, const async_callback_t& handler) {
     *         return add_async(http_method::GET, route, handler);
     *     }
     *     
     *     http_router& post_async(std::string_view route, const async_callback_t& handler) {
     *         return add_async(http_method::POST, route, handler);
     *     }
     *     
     *     // ... similar for other HTTP methods ...
     * };
     */

    /**
     * Example async handlers:
     * 
     * // 1. Async database query
     * server->get_async("/users/:id", [db_pool](const http_request& req) 
     *     -> net::awaitable<http_response> 
     * {
     *     auto id = req.find_uri_param("id");
     *     
     *     // Async database query (assuming async driver)
     *     auto user = co_await db_pool->async_query(
     *         "SELECT * FROM users WHERE id = $1", *id);
     *     
     *     if (user.empty()) {
     *         co_return http_response(404).json({{"error", "User not found"}});
     *     }
     *     
     *     co_return http_response(200).json({{"user", user[0]}});
     * });
     * 
     * // 2. Async external API call
     * server->get_async("/weather/:city", [](const http_request& req)
     *     -> net::awaitable<http_response>
     * {
     *     auto city = req.find_uri_param("city");
     *     
     *     // Async HTTP client call
     *     http_client client;
     *     auto weather = co_await client.get_async(
     *         std::format("https://api.weather.com/v1/current?city={}", *city));
     *     
     *     co_return http_response(200).json(weather);
     * });
     * 
     * // 3. Multiple async operations
     * server->post_async("/aggregate", [](const http_request& req)
     *     -> net::awaitable<http_response>
     * {
     *     // Parallel async operations
     *     auto [users, orders, products] = co_await std::tuple{
     *         db->async_query("SELECT COUNT(*) FROM users"),
     *         db->async_query("SELECT COUNT(*) FROM orders"),
     *         db->async_query("SELECT COUNT(*) FROM products")
     *     };
     *     
     *     co_return http_response(200).json({
     *         {"users", users},
     *         {"orders", orders},
     *         {"products", products}
     *     });
     * });
     * 
     * // 4. Delayed response (async timer)
     * server->get_async("/delayed", [](const http_request& req)
     *     -> net::awaitable<http_response>
     * {
     *     auto executor = co_await net::this_coro::executor;
     *     net::steady_timer timer(executor);
     *     timer.expires_after(std::chrono::seconds(5));
     *     
     *     co_await timer.async_wait(net::use_awaitable);
     *     
     *     co_return http_response(200).text("Delayed response after 5 seconds");
     * });
     */
}
