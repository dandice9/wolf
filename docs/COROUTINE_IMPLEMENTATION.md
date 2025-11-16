# Coroutine-Based Async Implementation in Wolf Web Server

## Overview

The Wolf web server now uses C++20 coroutines internally for handling HTTP requests. This provides a foundation for future async handler support while maintaining backward compatibility with existing synchronous handlers.

## Current Implementation

### Internal Coroutines

The `http_session::handle_request()` method is now a coroutine (`net::awaitable<void>`), which provides several benefits:

1. **Non-blocking I/O**: Uses `co_await` with `beast::http::async_write` for efficient async writes
2. **Cleaner Code**: Eliminates callback hell with linear async code flow
3. **Better Resource Utilization**: Threads aren't blocked waiting for I/O operations

### Code Structure

```cpp
net::awaitable<void> handle_request() {
    // ... request processing ...
    
    // Handler execution (currently synchronous)
    response_ = handler(req);
    
    // Non-blocking async write
    co_await beast::http::async_write(socket_, response_, net::use_awaitable);
    
    // ... cleanup ...
    co_return;
}
```

### Key Changes

1. **Added Coroutine Support Headers**:
   ```cpp
   #include <coroutine>
   #include <type_traits>
   ```

2. **Awaitable Type Traits**:
   ```cpp
   template<typename T>
   concept Awaitable = requires(T t) {
       { t.await_ready() } -> std::convertible_to<bool>;
       { t.await_suspend(std::coroutine_handle<>{}) };
       { t.await_resume() };
   };
   ```

3. **Coroutine Invocation**:
   ```cpp
   net::co_spawn(
       socket_.get_executor(),
       handle_request(),
       net::detached
   );
   ```

## Handler Support

### Current: Synchronous Handlers

Handlers are currently defined as:
```cpp
using callback_t = std::function<http_response(const http_request&)>;
```

Example:
```cpp
server->get("/api/user", [](const wolf::http_request& req) {
    return wolf::http_response(200)
        .json({{"user", "john"}});
});
```

### Future: Async Handler Support

To support truly async handlers, the implementation could be extended:

```cpp
// Define async callback type
using async_callback_t = std::function<net::awaitable<http_response>(const http_request&)>;

// In handle_request(), detect handler type:
if constexpr (is_awaitable_v<decltype(handler(req))>) {
    response_ = co_await handler(req);
} else {
    response_ = handler(req);
}
```

This would allow handlers like:
```cpp
server->get_async("/api/external", [](const wolf::http_request& req) 
    -> net::awaitable<wolf::http_response> 
{
    // Async HTTP call to external service
    auto result = co_await http_client.get("https://api.example.com/data");
    
    co_return wolf::http_response(200).json(result);
});
```

## Benefits

### Performance

1. **Non-blocking I/O**: Write operations don't block threads
2. **Better Concurrency**: Thread pool can handle more concurrent connections
3. **Reduced Latency**: Faster request-response cycles for I/O-bound operations

### Code Quality

1. **Linear Flow**: Async code reads like synchronous code
2. **Exception Safety**: Standard C++ exception handling works with coroutines
3. **Type Safety**: Compile-time checking of awaitable types

### Scalability

1. **Thread Efficiency**: Fewer threads needed for high connection counts
2. **Memory Usage**: Reduced stack usage per connection
3. **Throughput**: Better utilization of CPU and I/O resources

## Implementation Details

### WebSocket Handling

WebSocket upgrades are detected before coroutine processing:
```cpp
if (beast::websocket::is_upgrade(request_)) {
    auto ws_session = std::make_shared<websocket_session>(
        std::move(socket_), 
        router_.get_socket_handler()
    );
    ws_session->run(std::move(request_));
    co_return;
}
```

### Error Handling

The coroutine properly handles errors and connection cleanup:
```cpp
if(request_.need_eof()) {
    beast::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_send, ec);
} else {
    do_read();  // Continue reading for keep-alive connections
}
```

## Migration Guide

### Existing Code

No changes needed! All existing synchronous handlers continue to work:
```cpp
server->get("/old", [](const wolf::http_request& req) {
    return wolf::http_response(200).text("Still works!");
});
```

### Future Async Handlers

When async handler support is added, you'll be able to:
```cpp
server->get_async("/new", [](const wolf::http_request& req) 
    -> net::awaitable<wolf::http_response> 
{
    co_await some_async_operation();
    co_return wolf::http_response(200).text("Async!");
});
```

## Testing

All 30 existing tests pass with the coroutine implementation:
- ✅ Authentication tests
- ✅ Database operations
- ✅ HTTP route handling
- ✅ Session management

## Next Steps

To add full async handler support:

1. **Extend Router**: Add async handler registration methods
2. **Type Detection**: Use concepts to detect sync vs async handlers
3. **Handler Variants**: Support both callback types in router storage
4. **Conditional Await**: Use `if constexpr` to conditionally await handlers
5. **Documentation**: Update examples and API docs

## Performance Considerations

### When to Use Async Handlers (Future)

Use async handlers for:
- External API calls
- Database queries with async drivers
- File I/O operations
- Long-running computations that can be yielded

### When to Use Sync Handlers

Use sync handlers for:
- Simple request/response logic
- In-memory operations
- Quick computations
- Existing synchronous code

## Compiler Requirements

- **C++20**: Required for coroutines
- **Boost.Asio**: With coroutine support
- **Boost.Beast**: For HTTP/WebSocket support

## References

- [C++20 Coroutines](https://en.cppreference.com/w/cpp/language/coroutines)
- [Boost.Asio Coroutines](https://www.boost.org/doc/libs/1_83_0/doc/html/boost_asio/overview/composition/coroutine.html)
- [Boost.Beast Async Operations](https://www.boost.org/doc/libs/1_83_0/libs/beast/doc/html/beast/using_http/writing_composed_operations.html)
