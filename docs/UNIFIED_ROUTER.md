# Unified Router API

## Overview

Wolf now provides a **fully unified router** that accepts both synchronous and asynchronous handlers without requiring you to choose upfront. This eliminates the need for separate `wolf_router` and `wolf_async_router` types.

## Key Features

✅ **Single Router Type**: Just use `wolf_router` for everything  
✅ **Mix Sync & Async**: Use synchronous and asynchronous handlers in the same router  
✅ **Transparent**: Handlers are automatically wrapped to work seamlessly  
✅ **Type-Safe**: Full compile-time type checking  
✅ **Zero Overhead**: No runtime performance penalty for synchronous handlers

## Quick Start

```cpp
#include "wolf.hpp"

using namespace wolf;
namespace net = boost::asio;

int main() {
    web_server server(8080);
    
    // ✅ Synchronous handler
    server->get("/sync", [](const http_request& req) {
        return http_response(200).text("Hello");
    });
    
    // ✅ Asynchronous handler (notice the return type annotation)
    server->get("/async", [](const http_request& req) -> net::awaitable<http_response> {
        auto executor = co_await net::this_coro::executor;
        net::steady_timer timer(executor);
        timer.expires_after(std::chrono::milliseconds(100));
        co_await timer.async_wait(net::use_awaitable);
        
        co_return http_response(200).text("Async response");
    });
    
    server.listen();
}
```

## Handler Types

### Synchronous Handlers

**When to use**: For quick operations that don't require I/O (e.g., simple calculations, in-memory lookups)

```cpp
router.get("/hello", [](const http_request& req) {
    // Direct return
    return http_response(200).text("Hello, World!");
});
```

**Characteristics**:
- Returns `http_response` directly
- Executes immediately
- No `co_await` or `co_return`
- Perfect for simple endpoints

### Asynchronous Handlers

**When to use**: For I/O operations (database queries, external API calls, file operations)

```cpp
router.get("/db-query", [](const http_request& req) -> net::awaitable<http_response> {
    // Must explicitly annotate return type
    auto data = co_await database.query("SELECT * FROM users");
    co_return http_response(200).json(data);
});
```

**Characteristics**:
- Returns `net::awaitable<http_response>`
- **Must** annotate return type: `-> net::awaitable<http_response>`
- Uses `co_await` and `co_return`
- Non-blocking, scales well

## Implementation Details

### Type Erasure Approach

The unified router uses type erasure to store both synchronous and asynchronous handlers:

```cpp
template<typename PT>
class unified_handler {
public:
    using sync_fn = std::function<response_t(PT)>;
    using async_fn = std::function<net::awaitable<response_t>(PT)>;
    
    // Accepts both synchronous and asynchronous functions
    unified_handler(sync_fn fn);
    unified_handler(async_fn fn);
    
    // Always returns awaitable (sync handlers are wrapped)
    [[nodiscard]] net::awaitable<response_t> operator()(PT req) const;
};
```

### Handler Resolution

When a handler is called:

1. **Async handler**: Directly `co_await` the returned awaitable
2. **Sync handler**: Automatically wrapped in a coroutine that returns immediately

```cpp
[[nodiscard]] net::awaitable<response_t> operator()(PT req) const {
    if (is_async_) {
        co_return co_await async_handler_(req);  // Direct await
    } else {
        co_return handler_(req);  // Wrap sync result in awaitable
    }
}
```

## Migration Guide

### Before (Two Router Types)

```cpp
// Had to choose upfront
wolf_router sync_router;          // For sync handlers
wolf_async_router async_router;   // For async handlers

// Couldn't mix them
sync_router.get("/sync", sync_handler);
async_router.get("/async", async_handler);
```

### After (Unified Router)

```cpp
// Single router for everything
wolf_router router;

// Mix freely
router.get("/sync", sync_handler);
router.get("/async", async_handler);
```

## Examples

### Real-World Use Case

```cpp
web_server server(8080);

// Quick health check - synchronous
server->get("/health", [](const http_request& req) {
    return http_response(200).text("OK");
});

// Database query - asynchronous
server->get("/users", [](const http_request& req) -> net::awaitable<http_response> {
    auto conn = co_await db_pool.acquire();
    auto users = co_await conn.query("SELECT * FROM users");
    co_return http_response(200).json(users);
});

// In-memory cache lookup - synchronous
server->get("/config", [&cache](const http_request& req) {
    auto config = cache.get("app_config");
    return http_response(200).json(config);
});

// External API call - asynchronous
server->get("/weather", [](const http_request& req) -> net::awaitable<http_response> {
    auto weather_data = co_await http_client.get("https://api.weather.com/...");
    co_return http_response(200).json(weather_data);
});
```

### Parameterized Routes

Both sync and async handlers support parameters:

```cpp
// Synchronous with parameters
server->get("/items/:id", [](const http_request& req) {
    auto id = req.uri_params.at("id");
    return http_response(200).text("Item " + id);
});

// Asynchronous with parameters
server->get("/orders/:id", [](const http_request& req) -> net::awaitable<http_response> {
    auto id = req.uri_params.at("id");
    auto order = co_await db.find_order(id);
    co_return http_response(200).json(order);
});
```

## Performance Considerations

### Synchronous Handlers

- **Zero overhead**: Direct function call
- **Instant execution**: No coroutine machinery
- **Use for**: Quick operations < 1ms

### Asynchronous Handlers

- **Minimal overhead**: Coroutine frame allocation
- **Non-blocking**: Other requests can be processed
- **Use for**: I/O operations, network calls, database queries

### Best Practices

1. **Default to sync** for simple operations
2. **Use async** when doing I/O
3. **Don't mix blocking I/O with sync handlers** (defeats the purpose)
4. **Profile** if performance matters

## Common Pitfalls

### ❌ Forgot Return Type Annotation

```cpp
// WRONG - Won't compile
server->get("/async", [](const http_request& req) {
    co_await some_async_operation();
    co_return http_response(200);
});
```

**Error**: Cannot deduce return type from coroutine

**Fix**: Add `-> net::awaitable<http_response>`

```cpp
// CORRECT
server->get("/async", [](const http_request& req) -> net::awaitable<http_response> {
    co_await some_async_operation();
    co_return http_response(200);
});
```

### ❌ Blocking I/O in Sync Handler

```cpp
// BAD - Blocks the entire server
server->get("/slow", [](const http_request& req) {
    auto data = blocking_db_query();  // ❌ Blocks thread!
    return http_response(200).json(data);
});
```

**Problem**: Sync handlers block the thread. Use async for I/O.

```cpp
// GOOD - Non-blocking
server->get("/fast", [](const http_request& req) -> net::awaitable<http_response> {
    auto data = co_await async_db_query();  // ✅ Non-blocking
    co_return http_response(200).json(data);
});
```

## Testing

Both handler types can be tested using the same pattern:

```cpp
TEST_CASE("Unified router tests") {
    wolf_router router;
    net::io_context ioc;
    
    SECTION("Sync handler") {
        router.get("/sync", [](const http_request& req) {
            return http_response(200).text("OK");
        });
        
        auto [_, handler, __] = router.resolve(http_method::GET, "/sync");
        
        // Use co_spawn to run the handler
        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await handler(mock_request);
            REQUIRE(response.result() == http::status::ok);
        }, net::detached);
        
        ioc.run();
    }
    
    SECTION("Async handler") {
        router.get("/async", [](const http_request& req) -> net::awaitable<http_response> {
            co_return http_response(200).text("Async OK");
        });
        
        // Same testing pattern!
        auto [_, handler, __] = router.resolve(http_method::GET, "/async");
        
        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await handler(mock_request);
            REQUIRE(response.result() == http::status::ok);
        }, net::detached);
        
        ioc.run();
    }
}
```

## Summary

The unified router approach provides:

✅ **Simplicity**: One router type, no configuration needed  
✅ **Flexibility**: Mix sync and async handlers freely  
✅ **Type Safety**: Compile-time checks prevent errors  
✅ **Performance**: No overhead for sync handlers  
✅ **Seamless**: Handler types detected automatically

**Bottom line**: You no longer need to think about whether your router is "sync" or "async" - just write handlers naturally and Wolf handles the rest.
