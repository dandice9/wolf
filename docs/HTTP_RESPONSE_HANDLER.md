# Wolf Router - http_response Return Type Update

## Overview

The Wolf router has been enhanced to support handlers that return `wolf::http_response` directly, enabling the fluent API for all route handlers. This update makes it possible to use the modern, chainable response API throughout your application.

## What's New

### Before (Only response_t and string)
```cpp
server->get("/api/users", [](const wolf::http_request& req) -> wolf::response_t {
    wolf::response_t res{boost::beast::http::status::ok, 11};
    res.set(boost::beast::http::field::content_type, "application/json");
    res.body() = R"({"users": []})";
    res.prepare_payload();
    return res;
});

// Or string return
server->get("/health", [](const wolf::http_request& req) -> std::string {
    return "OK";
});
```

### After (Also supports http_response)
```cpp
server->get("/api/users", [](const wolf::http_request& req) -> wolf::http_response {
    return wolf::http_response(200).json(json::object{
        {"users", json::array{}}
    });
});

// All return types still supported!
server->get("/health", [](const wolf::http_request& req) -> std::string {
    return "OK";  // Still works!
});
```

## Return Type Options

The router now supports **three** return types for handlers:

### 1. `response_t` (Original)
```cpp
server->get("/api/old-style", [](const wolf::http_request& req) -> wolf::response_t {
    wolf::response_t res{boost::beast::http::status::ok, 11};
    res.set(boost::beast::http::field::content_type, "application/json");
    res.body() = R"({"message": "old style"})";
    res.prepare_payload();
    return res;
});
```

### 2. `std::string` (Simple)
```cpp
server->get("/api/simple", [](const wolf::http_request& req) -> std::string {
    return "Simple text response";
});
```

### 3. `http_response` (NEW - Fluent API)
```cpp
server->get("/api/modern", [](const wolf::http_request& req) -> wolf::http_response {
    return wolf::http_response(200).json(json::object{
        {"message", "modern fluent API"}
    });
});
```

## All HTTP Methods Supported

Every HTTP method now accepts handlers returning `http_response`:

```cpp
// GET
server->get("/resource", [](auto& req) -> wolf::http_response {
    return wolf::http_response(200).json(data);
});

// POST
server->post("/resource", [](auto& req) -> wolf::http_response {
    return wolf::http_response(201).json(result);
});

// PUT
server->put("/resource/:id", [](auto& req) -> wolf::http_response {
    return wolf::http_response(200).json(updated);
});

// DELETE
server->delete_("/resource/:id", [](auto& req) -> wolf::http_response {
    return wolf::http_response(204).text("");
});

// PATCH
server->patch("/resource/:id", [](auto& req) -> wolf::http_response {
    return wolf::http_response(200).json(patched);
});

// OPTIONS, HEAD, CONNECT, TRACE also supported
```

## Practical Examples

### RESTful API with http_response

```cpp
wolf::web_server server(8080);

// GET - List resources
server->get("/api/users", [](const wolf::http_request& req) -> wolf::http_response {
    json::array users = fetch_users();
    return wolf::http_response(200).json(users);
});

// POST - Create resource
server->post("/api/users", [](const wolf::http_request& req) -> wolf::http_response {
    auto body = req.get_json_body();
    auto user_id = create_user(body);
    
    return wolf::http_response(201)
        .header("Location", std::format("/api/users/{}", user_id))
        .json(json::object{
            {"success", true},
            {"id", user_id}
        });
});

// GET - Get single resource
server->get("/api/users/:id", [](const wolf::http_request& req) -> wolf::http_response {
    auto id = req.get_or("id", "0");
    
    auto user = find_user(id);
    if (!user) {
        return wolf::http_response(404).json(json::object{
            {"error", "Not Found"},
            {"message", "User not found"}
        });
    }
    
    return wolf::http_response(200).json(*user);
});

// PUT - Update resource
server->put("/api/users/:id", [](const wolf::http_request& req) -> wolf::http_response {
    auto id = req.get_or("id", "0");
    auto body = req.get_json_body();
    
    auto updated = update_user(id, body);
    
    return wolf::http_response(200).json(json::object{
        {"success", true},
        {"updated", updated}
    });
});

// DELETE - Delete resource
server->delete_("/api/users/:id", [](const wolf::http_request& req) -> wolf::http_response {
    auto id = req.get_or("id", "0");
    delete_user(id);
    
    return wolf::http_response(204).text("");
});
```

### Error Handling with http_response

```cpp
server->get("/api/data", [](const wolf::http_request& req) -> wolf::http_response {
    try {
        auto data = fetch_data();
        return wolf::http_response(200).json(data);
        
    } catch (const NotFoundException& e) {
        return wolf::http_response(404).json(json::object{
            {"error", "Not Found"},
            {"message", e.what()}
        });
        
    } catch (const ValidationException& e) {
        return wolf::http_response(422).json(json::object{
            {"error", "Validation Failed"},
            {"details", e.details()}
        });
        
    } catch (const std::exception& e) {
        return wolf::http_response(500).json(json::object{
            {"error", "Internal Server Error"},
            {"message", "An unexpected error occurred"}
        });
    }
});
```

### Complex Responses with Headers and Cookies

```cpp
server->post("/api/login", [](const wolf::http_request& req) -> wolf::http_response {
    auto body = req.get_json_body();
    
    // Authenticate user
    auto [success, session_id, user] = authenticate(body);
    
    if (!success) {
        return wolf::http_response(401)
            .header("WWW-Authenticate", "Bearer")
            .json(json::object{
                {"error", "Unauthorized"},
                {"message", "Invalid credentials"}
            });
    }
    
    return wolf::http_response(200)
        .header("X-RateLimit-Remaining", "99")
        .json(json::object{
            {"success", true},
            {"user", user}
        })
        .cookie("session_id", session_id, "/", "", 3600, true, true);
});
```

### Conditional Responses

```cpp
server->get("/api/resource", [](const wolf::http_request& req) -> wolf::http_response {
    auto format = req.get_or("format", "json");
    auto data = get_data();
    
    if (format == "json") {
        return wolf::http_response(200).json(data);
    } else if (format == "xml") {
        return wolf::http_response(200)
            .header("Content-Type", "application/xml")
            .text(convert_to_xml(data));
    } else if (format == "csv") {
        return wolf::http_response(200)
            .header("Content-Type", "text/csv")
            .text(convert_to_csv(data));
    } else {
        return wolf::http_response(400).json(json::object{
            {"error", "Bad Request"},
            {"message", "Unsupported format"},
            {"supported", json::array{"json", "xml", "csv"}}
        });
    }
});
```

## Implementation Details

### C++20 Concepts

The router uses C++20 concepts to provide type safety:

```cpp
template<typename T>
concept ResponseLike = std::derived_from<T, response_t> || std::same_as<T, response_t>;
```

This ensures that only valid response types can be used.

### Automatic Conversion

When you return `http_response`, it's automatically converted to `response_t`:

```cpp
template<RouteString T, ResponseLike R>
requires (!std::same_as<R, RT>)
http_router& add(http_method method, T&& route, const std::function<R(PT)>& handler) {
    return add(method, std::forward<T>(route), [handler](PT req) -> RT {
        return static_cast<RT>(handler(req));
    });
}
```

### Performance

There's **no performance overhead** - the conversion is a simple upcast from derived class to base class, which is a zero-cost operation in C++.

## Migration Guide

### Gradual Migration

You don't need to change all your handlers at once. All return types work together:

```cpp
// Mix and match!
server->get("/old", [](auto& req) -> wolf::response_t { /* ... */ });
server->get("/simple", [](auto& req) -> std::string { return "OK"; });
server->get("/new", [](auto& req) -> wolf::http_response { 
    return wolf::http_response(200).json(data); 
});
```

### Recommended Migration Path

1. **Keep existing handlers** working as-is
2. **New handlers** use `http_response` for fluent API
3. **Refactor old handlers** gradually when updating them

### Before and After Examples

#### Old Style
```cpp
server->post("/api/users", [](const wolf::http_request& req) {
    wolf::response_t res{boost::beast::http::status::created, 11};
    res.set(boost::beast::http::field::content_type, "application/json");
    res.set("X-API-Version", "1.0");
    
    json::object result = {{"id", 123}};
    res.body() = json::serialize(result);
    res.prepare_payload();
    
    return res;
});
```

#### New Style
```cpp
server->post("/api/users", [](const wolf::http_request& req) -> wolf::http_response {
    json::object result = {{"id", 123}};
    
    return wolf::http_response(201)
        .header("X-API-Version", "1.0")
        .json(result);
});
```

## Benefits

✅ **Cleaner code** - Less boilerplate, more expressive  
✅ **Type-safe** - Compile-time checking with C++20 concepts  
✅ **Chainable** - Natural method chaining  
✅ **Flexible** - Use the style that fits your needs  
✅ **Backward compatible** - Existing code continues to work  
✅ **Zero overhead** - No performance cost  

## Testing

The new functionality is thoroughly tested:

- ✅ All HTTP methods (GET, POST, PUT, DELETE, PATCH, etc.)
- ✅ Route parameters with http_response
- ✅ Custom headers
- ✅ Cookies
- ✅ Different status codes
- ✅ JSON, text, and HTML responses
- ✅ Complex method chaining
- ✅ Error responses

See `tests/router_http_response_test.cpp` for complete test coverage.

## Examples

- **Basic Usage**: `examples/http_response_handler_demo.cpp`
- **Fluent API**: `examples/fluent_response_example.cpp`
- **Quick Reference**: `examples/quick_reference.cpp`

## Summary

The Wolf router now provides maximum flexibility:

| Return Type | Use Case | Example |
|-------------|----------|---------|
| `response_t` | Full control, legacy code | Manual response building |
| `std::string` | Simple text responses | Quick endpoints |
| `http_response` | Modern fluent API | Recommended for new code |

All three styles work together seamlessly in the same application!
