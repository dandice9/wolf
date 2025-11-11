# Wolf HTTP Response - Quick Cheat Sheet

## Basic Pattern

```cpp
return wolf::http_response(STATUS_CODE).METHOD(data);
```

## Status Codes

| Code | Meaning | Use Case |
|------|---------|----------|
| 200 | OK | Successful GET, PUT, PATCH |
| 201 | Created | Successful POST (resource created) |
| 204 | No Content | Successful DELETE |
| 400 | Bad Request | Invalid input |
| 401 | Unauthorized | Missing/invalid authentication |
| 403 | Forbidden | Insufficient permissions |
| 404 | Not Found | Resource doesn't exist |
| 422 | Unprocessable Entity | Validation failed |
| 500 | Internal Server Error | Server error |

## Common Patterns

### Success Responses

```cpp
// GET - Return data
wolf::http_response(200).json(data)

// POST - Created
wolf::http_response(201).json(json::object{{"id", 123}})

// PUT/PATCH - Updated
wolf::http_response(200).json(json::object{{"updated", true}})

// DELETE - Deleted
wolf::http_response(204).text("")
```

### Error Responses

```cpp
// Not found
wolf::http_response(404).json(json::object{
    {"error", "Not Found"},
    {"message", "Resource not found"}
})

// Validation error
wolf::http_response(422).json(json::object{
    {"success", false},
    {"errors", json::array{...}}
})

// Unauthorized
wolf::http_response(401)
    .header("WWW-Authenticate", "Bearer")
    .json(json::object{{"error", "Unauthorized"}})

// Bad request
wolf::http_response(400).json(json::object{
    {"error", "Bad Request"},
    {"message", "Missing parameter"}
})

// Server error
wolf::http_response(500).json(json::object{
    {"error", "Internal Server Error"}
})
```

## Methods

| Method | Content-Type | Example |
|--------|-------------|---------|
| `.json(obj)` | application/json | `.json(json::object{{"key", "val"}})` |
| `.json(arr)` | application/json | `.json(json::array{1, 2, 3})` |
| `.text(str)` | text/plain | `.text("Hello")` |
| `.html(str)` | text/html | `.html("<h1>Hi</h1>")` |
| `.header(k, v)` | - | `.header("X-Key", "value")` |
| `.cookie(...)` | - | `.cookie("name", "val", "/", "", 3600, true, false)` |
| `.status(code)` | - | `.status(200)` |

## Method Chaining

```cpp
wolf::http_response(200)
    .header("X-API-Version", "1.0")
    .header("X-RateLimit", "100")
    .json(data)
    .cookie("session", "abc123", "/", "", 3600, true, false)
```

## Cookie Parameters

```cpp
.cookie(
    "name",      // key
    "value",     // value
    "/",         // path
    "",          // domain
    3600,        // max-age (seconds)
    true,        // HttpOnly
    false        // Secure
)
```

## Quick Examples

```cpp
// JSON success
return wolf::http_response(200).json(json::object{{"status", "ok"}});

// Created with ID
return wolf::http_response(201).json(json::object{{"id", 123}});

// Not found error
return wolf::http_response(404).json(json::object{{"error", "Not Found"}});

// With custom header
return wolf::http_response(200)
    .header("X-Request-ID", "abc")
    .json(data);

// With cookie
return wolf::http_response(200)
    .json(data)
    .cookie("session", "xyz", "/", "", 3600, true, false);

// Plain text
return wolf::http_response(200).text("OK");

// HTML page
return wolf::http_response(200).html("<h1>Hello</h1>");

// No content (delete)
return wolf::http_response(204).text("");

// Array response
return wolf::http_response(200).json(json::array{1, 2, 3});
```

## RESTful Endpoint Template

```cpp
// CREATE (POST)
server->post("/api/resource", [](const wolf::http_request& req) {
    auto body = req.get_json_body();
    // ... create logic ...
    return wolf::http_response(201).json(json::object{{"id", id}});
});

// READ (GET)
server->get("/api/resource/:id", [](const wolf::http_request& req) {
    auto id = req.get_or("id", "0");
    // ... fetch logic ...
    return wolf::http_response(200).json(data);
});

// UPDATE (PUT/PATCH)
server->put("/api/resource/:id", [](const wolf::http_request& req) {
    auto id = req.get_or("id", "0");
    auto body = req.get_json_body();
    // ... update logic ...
    return wolf::http_response(200).json(json::object{{"updated", true}});
});

// DELETE
server->delete_("/api/resource/:id", [](const wolf::http_request& req) {
    auto id = req.get_or("id", "0");
    // ... delete logic ...
    return wolf::http_response(204).text("");
});

// LIST (GET)
server->get("/api/resources", [](const wolf::http_request& req) {
    // ... list logic ...
    return wolf::http_response(200).json(array);
});
```

## Error Handling Pattern

```cpp
server->get("/api/endpoint", [](const wolf::http_request& req) {
    try {
        // Your logic here
        return wolf::http_response(200).json(result);
    } catch (const NotFoundException& e) {
        return wolf::http_response(404).json(json::object{
            {"error", "Not Found"},
            {"message", e.what()}
        });
    } catch (const ValidationException& e) {
        return wolf::http_response(422).json(json::object{
            {"error", "Validation Failed"},
            {"message", e.what()}
        });
    } catch (const std::exception& e) {
        return wolf::http_response(500).json(json::object{
            {"error", "Internal Server Error"},
            {"message", "An unexpected error occurred"}
        });
    }
});
```

## Tips

✅ **DO:**
- Use semantic status codes
- Return JSON for APIs
- Include error details
- Chain methods for clean code
- Use 201 for POST success
- Use 204 for DELETE success

❌ **DON'T:**
- Return 200 for errors
- Omit error messages
- Use 500 for client errors
- Mix content types inconsistently

## One-Liners

```cpp
// Success
return wolf::http_response(200).json(data);

// Created
return wolf::http_response(201).json(json::object{{"id", 1}});

// Error
return wolf::http_response(404).json(json::object{{"error", "Not Found"}});

// Text
return wolf::http_response(200).text("OK");

// HTML
return wolf::http_response(200).html("<h1>Hi</h1>");
```
