# Async Coroutine Test Coverage

## Overview
Comprehensive test suite for Wolf Web Framework's async coroutine functionality using C++20 coroutines with Boost.Asio.

## Test Statistics
- **Total Assertions**: 75
- **Test Cases**: 7
- **All Tests**: ✅ PASSING

## Test Structure

### 1. Type Traits and Concepts (`Async Coroutine - Type traits`)
**Assertions**: 11

Tests compile-time type checking for awaitable types:
- `is_awaitable_v` trait detection for various types
- `Awaitable` concept validation using `static_assert`
- Verifies distinction between awaitable and non-awaitable types

**Key Coverage**:
- ✅ `net::awaitable<void>`, `net::awaitable<int>`, `net::awaitable<http_response>`
- ✅ Non-awaitable types: `void`, `int`, `std::string`, `http_response`
- ✅ Compile-time concept checking

### 2. Router with Async Handlers (`Async Coroutine - Router with async handlers`)
**Assertions**: 12

Tests router registration and resolution with async handlers:
- Async handler registration that returns `net::awaitable<http_response>`
- Parameterized routes with async handlers
- Multiple HTTP methods (GET, POST, PUT, DELETE)
- JSON response handling
- Complex nested routes with multiple parameters

**Key Coverage**:
- ✅ Basic async route registration
- ✅ Parameter extraction (`/users/:id`)
- ✅ All HTTP methods (GET, POST, PUT, DELETE)
- ✅ Nested routes (`/api/:version/users/:userId/posts/:postId`)
- ✅ JSON responses with boost::json::object

### 3. Handler Execution (`Async Coroutine - Handler execution`)
**Assertions**: 10

Tests actual execution of async handlers:
- Simple async handler execution with `co_await`
- Simulated delays using `net::steady_timer`
- Multiple concurrent async handlers
- JSON request body processing
- Error response handling

**Key Coverage**:
- ✅ Basic coroutine execution with `net::co_spawn`
- ✅ Timer-based delays (`std::chrono::milliseconds`)
- ✅ Concurrent execution (5 simultaneous handlers)
- ✅ JSON echo functionality
- ✅ Error responses (400 Bad Request)

### 4. Fluent API Integration (`Async Coroutine - Fluent API integration`)
**Assertions**: 11

Tests fluent API method chaining with async handlers:
- Chained header, cookie, and JSON methods
- HTML response generation
- Status code chaining

**Key Coverage**:
- ✅ Method chaining: `.header().cookie().json()`
- ✅ Custom headers (`X-Custom-Header`)
- ✅ Cookie setting with async responses
- ✅ HTML content type
- ✅ Status code modification (404, 201)

### 5. RESTful API Patterns (`Async Coroutine - RESTful API patterns`)
**Assertions**: 5

Tests complete CRUD operations with async handlers:
- CREATE (POST)
- READ list (GET collection)
- READ single (GET with ID)
- UPDATE (PUT)
- DELETE (DELETE)

**Key Coverage**:
- ✅ POST `/api/users` - Create with 201 status
- ✅ GET `/api/users` - List with array response
- ✅ GET `/api/users/:id` - Single resource
- ✅ PUT `/api/users/:id` - Update with JSON body
- ✅ DELETE `/api/users/:id` - 204 No Content

### 6. Error Handling (`Async Coroutine - Error handling`)
**Assertions**: 5

Tests error scenarios and timeout simulation:
- Exception handling in async handlers
- 500 Internal Server Error responses
- Timeout simulation with timers

**Key Coverage**:
- ✅ Error response generation (500 status)
- ✅ Success case verification (200 status)
- ✅ Timeout validation (50ms+ duration)
- ✅ Error message formatting

### 7. Query and Cookie Handling (`Async Coroutine - Query and cookie handling`)
**Assertions**: 21

Tests parameter parsing in async context:
- Query parameter extraction and processing
- Cookie reading and setting
- Optional parameter handling with defaults

**Key Coverage**:
- ✅ Query params: `page=2&limit=20`
- ✅ Default values with `.value_or()`
- ✅ Cookie extraction from request
- ✅ Cookie setting in response
- ✅ JSON response with extracted values

## Implementation Details

### Async Router Type
```cpp
wolf::wolf_router<true>  // Template parameter true = async mode
```

### Async Handler Signature
```cpp
auto handler = [](const http_request& req) -> net::awaitable<http_response> {
    // Async operations using co_await
    co_return http_response(200).json(obj);
};
```

### Execution Pattern
```cpp
net::co_spawn(ioc, [&]() -> net::awaitable<void> {
    auto response = co_await async_handler(req);
    // Verify response
}, net::detached);
ioc.run();  // Run event loop
```

## Key Features Tested

### C++20 Coroutines
- ✅ `co_await` for async operations
- ✅ `co_return` for value return
- ✅ `net::awaitable<T>` return types
- ✅ `net::use_awaitable` for async operations

### Boost.Asio Integration
- ✅ `net::io_context` event loop
- ✅ `net::co_spawn` for coroutine execution
- ✅ `net::steady_timer` for delays
- ✅ `net::detached` completion token

### Async Response Building
- ✅ Fluent API with method chaining
- ✅ JSON object creation with `boost::json::object`
- ✅ Multiple content types (JSON, HTML, text)
- ✅ Custom headers and cookies
- ✅ Status code setting

### Concurrency
- ✅ Multiple concurrent coroutines
- ✅ Atomic counters for verification
- ✅ Timer-based synchronization
- ✅ Independent execution contexts

## Test Patterns

### 1. Basic Execution Test
```cpp
net::co_spawn(ioc, [&]() -> net::awaitable<void> {
    auto response = co_await async_handler(req);
    REQUIRE(response.result() == http::status::ok);
}, net::detached);
ioc.run();
```

### 2. Concurrent Execution Test
```cpp
for (int i = 1; i <= 5; ++i) {
    net::co_spawn(ioc, [&, i]() -> net::awaitable<void> {
        auto response = co_await async_handler(i);
        // Verify response
    }, net::detached);
}
ioc.run();
```

### 3. Timer-Based Delay Test
```cpp
auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
    auto executor = co_await net::this_coro::executor;
    net::steady_timer timer(executor);
    timer.expires_after(std::chrono::milliseconds(50));
    co_await timer.async_wait(net::use_awaitable);
    co_return http_response(200).text("Delayed");
};
```

## Comparison with Sync Tests

| Feature | Sync Tests | Async Tests |
|---------|-----------|-------------|
| Router type | `wolf_router<false>` | `wolf_router<true>` |
| Return type | `http_response` | `net::awaitable<http_response>` |
| Handler execution | Direct call | `co_await` call |
| Concurrency | Sequential | Parallel with `co_spawn` |
| Delays | `std::this_thread::sleep_for` | `net::steady_timer` + `co_await` |
| Error handling | Exception catch | Coroutine error propagation |

## Build Configuration

### CMakeLists.txt Addition
```cmake
add_executable(async_coroutine_tests async_coroutine_test.cpp)
target_link_libraries(async_coroutine_tests 
    PRIVATE 
    Catch2::Catch2WithMain
    ${Boost_LIBRARIES}
)
add_test(NAME async_coroutine_tests COMMAND async_coroutine_tests)
```

### Compiler Requirements
- C++23 standard (`CMAKE_CXX_STANDARD 23`)
- Boost libraries: json, url, asio
- Catch2 v3 testing framework

## Best Practices Demonstrated

1. **Explicit JSON Object Creation**: Avoiding ambiguous initializer lists
   ```cpp
   boost::json::object obj;
   obj["key"] = "value";
   co_return http_response(200).json(obj);
   ```

2. **Proper Event Loop Management**: Always call `ioc.run()` after spawning coroutines

3. **Atomic Counters**: Use `std::atomic` for thread-safe verification

4. **Timer-Based Delays**: Prefer `net::steady_timer` over blocking sleeps

5. **Optional Parameter Handling**: Use `.value_or()` for default values

## Future Test Extensions

Potential areas for additional async coroutine test coverage:

1. **Database Operations**: Mock async database queries with delays
2. **External API Calls**: Simulate HTTP client requests
3. **WebSocket Async Handlers**: Test WebSocket upgrades with coroutines
4. **File I/O**: Async file reading/writing operations
5. **Error Propagation**: Exception handling across coroutine boundaries
6. **Cancellation**: Test coroutine cancellation scenarios
7. **Backpressure**: Test handling of slow consumers
8. **Connection Pooling**: Test resource management with coroutines

## Summary

The async coroutine test suite provides comprehensive coverage of Wolf's C++20 coroutine integration:

- ✅ **75 assertions** covering all major async functionality
- ✅ **Type safety** with concepts and static assertions
- ✅ **Fluent API** compatibility in async context
- ✅ **RESTful patterns** with CRUD operations
- ✅ **Concurrent execution** with multiple coroutines
- ✅ **Error handling** and timeout scenarios
- ✅ **Parameter handling** (URI, query, cookies)

All tests pass successfully, demonstrating robust async functionality in the Wolf Web Framework.
