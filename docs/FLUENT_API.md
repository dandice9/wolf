# Wolf HTTP Response - Fluent API Guide

## Overview

The `wolf::http_response` class now supports a fluent API pattern that allows you to chain method calls for creating HTTP responses. This makes response building more intuitive and concise.

## Basic Usage

### Creating Responses with Status Codes

```cpp
// Using integer status code
auto res = wolf::http_response(201);

// Using Beast HTTP status enum
auto res = wolf::http_response(boost::beast::http::status::created);

// Default constructor (200 OK)
auto res = wolf::http_response();
```

## Fluent API Methods

### JSON Responses

The most common use case - returning JSON data:

```cpp
// JSON object
server->get("/api/user", [](const wolf::http_request& req) {
    json::object user = {
        {"id", 1},
        {"name", "John Doe"},
        {"email", "john@example.com"}
    };
    return wolf::http_response(200).json(user);
});

// JSON array
server->get("/api/users", [](const wolf::http_request& req) {
    json::array users = {
        json::object{{"id", 1}, {"name", "Alice"}},
        json::object{{"id", 2}, {"name", "Bob"}}
    };
    return wolf::http_response(200).json(users);
});

// Empty JSON object
server->post("/api/resource", [](const wolf::http_request& req) {
    return wolf::http_response(201).json(json::object{});
});
```

### Text Responses

```cpp
server->get("/api/hello", [](const wolf::http_request& req) {
    return wolf::http_response(200).text("Hello, World!");
});
```

### HTML Responses

```cpp
server->get("/", [](const wolf::http_request& req) {
    return wolf::http_response(200).html(R"(
        <!DOCTYPE html>
        <html>
        <head><title>Welcome</title></head>
        <body><h1>Hello from Wolf!</h1></body>
        </html>
    )");
});
```

### Custom Headers

```cpp
server->get("/api/data", [](const wolf::http_request& req) {
    json::object data = {{"message", "success"}};
    
    return wolf::http_response(200)
        .json(data)
        .header("X-API-Version", "1.0")
        .header("X-RateLimit-Remaining", "99")
        .header("X-Request-ID", "12345");
});
```

### Cookies

```cpp
server->get("/api/login", [](const wolf::http_request& req) {
    json::object response = {
        {"success", true},
        {"message", "Login successful"}
    };
    
    return wolf::http_response(200)
        .json(response)
        .cookie("session_id", "abc123", "/", "", 3600, true, false);
        //     key          value     path domain max_age http_only secure
});
```

Cookie parameters:
- `key`: Cookie name
- `value`: Cookie value
- `path`: Cookie path (default: "/")
- `domain`: Cookie domain (default: "")
- `max_age`: Max age in seconds (default: -1, no expiry)
- `http_only`: HttpOnly flag (default: true)
- `secure`: Secure flag (default: false)

### File Downloads

```cpp
server->get("/api/download", [](const wolf::http_request& req) {
    std::string file_content = "File contents here...";
    
    wolf::http_response res(200);
    res.body() = file_content;
    return res.send_file("document.pdf", "application/pdf");
});
```

### Status Method

Change status code in the middle of a chain:

```cpp
server->get("/api/resource", [](const wolf::http_request& req) {
    bool found = check_resource();
    
    if (found) {
        return wolf::http_response()
            .status(200)
            .json(json::object{{"message", "Found"}});
    } else {
        return wolf::http_response()
            .status(404)
            .json(json::object{{"error", "Not Found"}});
    }
});
```

## Complete Method Reference

| Method | Description | Returns |
|--------|-------------|---------|
| `json(json::value)` | Set JSON body with application/json content-type | `http_response&` |
| `json(json::object)` | Set JSON object body | `http_response&` |
| `json(json::array)` | Set JSON array body | `http_response&` |
| `text(string_view)` | Set plain text body with text/plain content-type | `http_response&` |
| `html(string_view)` | Set HTML body with text/html content-type | `http_response&` |
| `header(key, value)` | Add custom header | `http_response&` |
| `cookie(...)` | Set cookie with options | `http_response&` |
| `send_file(filename, content_type)` | Set content-disposition for file download | `http_response&` |
| `status(int)` | Set HTTP status code | `http_response&` |
| `status(beast::http::status)` | Set HTTP status | `http_response&` |

## Common HTTP Status Codes

```cpp
// Success
200 - OK
201 - Created
204 - No Content

// Client Errors
400 - Bad Request
401 - Unauthorized
403 - Forbidden
404 - Not Found
422 - Unprocessable Entity

// Server Errors
500 - Internal Server Error
503 - Service Unavailable
```

## Real-World Examples

### RESTful API Endpoints

#### Create Resource (201 Created)

```cpp
server->post("/api/users", [](const wolf::http_request& req) {
    auto body = req.get_json_body();
    
    // Create user logic here...
    
    json::object response = {
        {"success", true},
        {"message", "User created successfully"},
        {"user_id", 123}
    };
    
    return wolf::http_response(201).json(response);
});
```

#### Validation Error (422 Unprocessable Entity)

```cpp
server->post("/api/register", [](const wolf::http_request& req) {
    auto body = req.get_json_body();
    
    // Validate input...
    
    json::object error = {
        {"success", false},
        {"errors", json::array{
            json::object{{"field", "email"}, {"message", "Invalid format"}},
            json::object{{"field", "password"}, {"message", "Too short"}}
        }}
    };
    
    return wolf::http_response(422).json(error);
});
```

#### Resource Not Found (404)

```cpp
server->get("/api/users/:id", [](const wolf::http_request& req) {
    auto id = req.get_or("id", "0");
    
    // Try to find user...
    bool found = find_user(id);
    
    if (!found) {
        json::object error = {
            {"error", "Not Found"},
            {"message", "User not found"},
            {"user_id", id}
        };
        return wolf::http_response(404).json(error);
    }
    
    json::object user = get_user_data(id);
    return wolf::http_response(200).json(user);
});
```

#### Delete Resource (204 No Content)

```cpp
server->delete_("/api/users/:id", [](const wolf::http_request& req) {
    auto id = req.get_or("id", "0");
    
    // Delete user logic...
    delete_user(id);
    
    return wolf::http_response(204).text("");
});
```

### Complex Chaining Example

```cpp
server->get("/api/profile", [](const wolf::http_request& req) {
    // Check authentication
    auto token = req.get_or("token", "");
    if (token.empty()) {
        json::object error = {{"error", "Unauthorized"}};
        return wolf::http_response(401)
            .header("WWW-Authenticate", "Bearer")
            .json(error);
    }
    
    // Get user profile
    json::object profile = {
        {"id", 1},
        {"name", "John Doe"},
        {"email", "john@example.com"},
        {"created_at", "2024-01-01"}
    };
    
    // Return with custom headers and cookie
    return wolf::http_response(200)
        .header("X-API-Version", "1.0")
        .header("X-Request-ID", generate_request_id())
        .header("X-RateLimit-Limit", "1000")
        .header("X-RateLimit-Remaining", "999")
        .json(profile)
        .cookie("last_access", std::to_string(std::time(nullptr)), "/", "", 86400, true, false);
});
```

### Error Handling Pattern

```cpp
server->get("/api/data", [](const wolf::http_request& req) {
    try {
        auto data = fetch_data();
        return wolf::http_response(200).json(data);
    } catch (const DatabaseException& e) {
        json::object error = {
            {"error", "Database Error"},
            {"message", e.what()},
            {"timestamp", std::time(nullptr)}
        };
        return wolf::http_response(500).json(error);
    } catch (const std::exception& e) {
        json::object error = {
            {"error", "Internal Server Error"},
            {"message", "An unexpected error occurred"}
        };
        return wolf::http_response(500).json(error);
    }
});
```

### Content Negotiation

```cpp
server->get("/api/resource", [](const wolf::http_request& req) {
    auto accept = req.get_or("format", "json");
    
    if (accept == "json") {
        json::object data = {{"message", "JSON response"}};
        return wolf::http_response(200).json(data);
    } else if (accept == "text") {
        return wolf::http_response(200).text("Plain text response");
    } else if (accept == "html") {
        return wolf::http_response(200).html("<h1>HTML response</h1>");
    } else {
        json::object error = {
            {"error", "Not Acceptable"},
            {"message", "Supported formats: json, text, html"}
        };
        return wolf::http_response(406).json(error);
    }
});
```

## Best Practices

1. **Always set appropriate status codes**: Use semantic HTTP status codes (200, 201, 404, 500, etc.)

2. **Use JSON for API responses**: Modern APIs typically return JSON

3. **Include error details**: When returning errors, provide useful information

4. **Set custom headers for metadata**: Use headers for API versions, rate limits, etc.

5. **Chain methods logically**: The fluent API allows clean, readable response building

6. **Use appropriate content types**: `.json()` sets `application/json`, `.text()` sets `text/plain`, `.html()` sets `text/html`

## Migration from Old API

**Before:**
```cpp
wolf::response_t res{beast::http::status::ok, 11};
res.set(beast::http::field::content_type, "application/json");
res.body() = json::serialize(data);
res.prepare_payload();
return res;
```

**After:**
```cpp
return wolf::http_response(200).json(data);
```

## Performance Notes

- All methods return references (`http_response&`), avoiding unnecessary copies
- Methods are marked with `[[nodiscard]]` to encourage proper usage
- The fluent API is just as efficient as the manual approach

## See Also

- [wolf/src/web_server.hpp](../src/web_server.hpp) - Full implementation
- [wolf/examples/fluent_response_example.cpp](../examples/fluent_response_example.cpp) - Complete examples
- [wolf/tests/http_response_test.cpp](../tests/http_response_test.cpp) - Test cases
