# Wolf Router - Seamless Sync/Async Support

## Overview

Wolf Web Framework now provides a cleaner, more intuitive API for working with both synchronous and asynchronous handlers without requiring explicit template parameters in most cases.

## Router Types

### 1. **`wolf_router`** (Default - Synchronous)
The default router for synchronous handlers. Perfect for simple applications and when you don't need async/await.

```cpp
#include "wolf.hpp"

wolf::web_server server(8080);  // Uses wolf_router by default

server->get("/hello", [](const wolf::http_request& req) {
    return wolf::http_response(200).text("Hello, World!");
});

server.start();
```

### 2. **`wolf_async_router`** (Asynchronous with Coroutines)
Use this when you need C++20 coroutine support for async operations.

```cpp
#include "wolf.hpp"

wolf::web_server<wolf::wolf_async_router> server(8080);

server->get("/async-hello", [](const wolf::http_request& req) -> net::awaitable<wolf::http_response> {
    // Perform async operations
    auto executor = co_await net::this_coro::executor;
    net::steady_timer timer(executor);
    timer.expires_after(std::chrono::milliseconds(100));
    co_await timer.async_wait(net::use_awaitable);
    
    co_return wolf::http_response(200).text("Async Hello!");
});

server.start();
```

## Key Improvements

### Before (Complex)
```cpp
// Had to explicitly specify template parameter
wolf::web_server<true> async_server(8080);   // Async
wolf::web_server<false> sync_server(8081);   // Sync

// Or use wolf_router<true/false>
wolf::wolf_router<true> async_router;
wolf::wolf_router<false> sync_router;
```

### After (Clean & Intuitive)
```cpp
// Default is sync - no template needed!
wolf::web_server server(8080);

// Explicit async when needed - clear intent
wolf::web_server<wolf::wolf_async_router> async_server(8081);

// Or with router directly
wolf::wolf_router sync_router;          // Synchronous
wolf::wolf_async_router async_router;   // Asynchronous
```

## Mixed Handler Support

The router automatically handles both sync and async handlers seamlessly:

### Synchronous Handlers
```cpp
wolf::wolf_router router;

// Returns http_response directly
router.get("/sync", [](const wolf::http_request& req) {
    return wolf::http_response(200).json({{"type", "sync"}});
});

// Returns std::string (automatically wrapped)
router.get("/text", [](const wolf::http_request& req) {
    return "Plain text response";
});

// Returns response_t (base type)
router.get("/base", [](const wolf::http_request& req) {
    wolf::response_t res;
    res.result(boost::beast::http::status::ok);
    res.body() = "Base response";
    res.prepare_payload();
    return res;
});
```

### Asynchronous Handlers
```cpp
wolf::wolf_async_router async_router;

// Returns awaitable<http_response>
async_router.get("/async", [](const wolf::http_request& req) -> net::awaitable<wolf::http_response> {
    co_return wolf::http_response(200).json({{"type", "async"}});
});

// Async with delays
async_router.get("/delayed", [](const wolf::http_request& req) -> net::awaitable<wolf::http_response> {
    auto executor = co_await net::this_coro::executor;
    net::steady_timer timer(executor);
    timer.expires_after(std::chrono::seconds(1));
    co_await timer.async_wait(net::use_awaitable);
    
    co_return wolf::http_response(200).text("Delayed response");
});
```

## Automatic Handler Conversion

The router intelligently converts handler return types:

| Handler Returns | Sync Router | Async Router |
|----------------|-------------|--------------|
| `http_response` | ✅ Direct | ✅ Wrapped in coroutine |
| `response_t` | ✅ Direct | ✅ Wrapped & converted |
| `std::string` | ✅ Wrapped in response | ✅ Wrapped in response & coroutine |
| `awaitable<http_response>` | ❌ Not supported | ✅ Direct |

## Complete Examples

### Simple Sync Server
```cpp
#include "wolf.hpp"

int main() {
    wolf::web_server server(8080);
    
    server->get("/", [](const wolf::http_request& req) {
        return wolf::http_response(200).html("<h1>Welcome!</h1>");
    });
    
    server->get("/api/user/:id", [](const wolf::http_request& req) {
        auto id = req.find_uri_param("id").value_or("unknown");
        boost::json::object data;
        data["id"] = id;
        data["name"] = "User " + id;
        return wolf::http_response(200).json(data);
    });
    
    server.start();
    return 0;
}
```

### Advanced Async Server
```cpp
#include "wolf.hpp"

int main() {
    wolf::web_server<wolf::wolf_async_router> server(8080);
    
    server->get("/", [](const wolf::http_request& req) -> net::awaitable<wolf::http_response> {
        co_return wolf::http_response(200).html("<h1>Async Welcome!</h1>");
    });
    
    server->get("/api/data", [](const wolf::http_request& req) -> net::awaitable<wolf::http_response> {
        // Simulate async database query
        auto executor = co_await net::this_coro::executor;
        net::steady_timer timer(executor);
        timer.expires_after(std::chrono::milliseconds(50));
        co_await timer.async_wait(net::use_awaitable);
        
        boost::json::object data;
        data["message"] = "Fetched from async source";
        data["timestamp"] = std::time(nullptr);
        
        co_return wolf::http_response(200).json(data);
    });
    
    server.start();
    return 0;
}
```

## Benefits

1. **✅ Clearer Intent**: `wolf_router` vs `wolf_async_router` makes the code's async nature obvious
2. **✅ Type Safety**: C++20 concepts ensure correct handler signatures at compile time
3. **✅ Flexibility**: Mix sync and async handlers with automatic conversion
4. **✅ Backward Compatible**: Existing sync code works without changes
5. **✅ No Boilerplate**: No need to manually wrap handlers or specify complex template parameters

## Migration Guide

### If you used `wolf_router<false>` (Sync)
```cpp
// Before
wolf::wolf_router<false> router;

// After - just remove the template
wolf::wolf_router router;
```

### If you used `wolf_router<true>` (Async)
```cpp
// Before
wolf::wolf_router<true> router;

// After - use the async alias
wolf::wolf_async_router router;
```

### If you used `web_server<false>` or `web_server<true>`
```cpp
// Before
wolf::web_server<false> sync_server(8080);
wolf::web_server<true> async_server(8081);

// After
wolf::web_server sync_server(8080);  // Default is sync
wolf::web_server<wolf::wolf_async_router> async_server(8081);
```

## Technical Details

- **Default Router**: `wolf_router` = `http_router<http_response, http_request>`
- **Async Router**: `wolf_async_router` = `http_router<net::awaitable<http_response>, http_request>`
- **Handler Detection**: Uses C++20 `is_awaitable_v` trait to detect async handlers
- **Automatic Wrapping**: Sync handlers on async routers are automatically wrapped in coroutines
- **Zero Overhead**: Template metaprogramming ensures no runtime overhead

## Recommendation

- **Use `wolf_router`** for most applications - simpler and easier to understand
- **Use `wolf_async_router`** when you need:
  - Async database operations
  - External API calls
  - Long-running computations without blocking
  - High concurrency with efficient resource usage

The framework handles the complexity so you can focus on building your application! 🚀
