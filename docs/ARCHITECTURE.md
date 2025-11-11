# Wolf HTTP Response - Fluent API Architecture

## Class Hierarchy

```
response_t (Boost.Beast)
    ↑
    │ inherits
    │
http_response (Wolf)
    │
    ├── Constructors
    │   ├── http_response()                    → 200 OK
    │   ├── http_response(int)                 → Custom status
    │   └── http_response(beast::http::status) → Custom status
    │
    ├── Content Methods (return http_response&)
    │   ├── .json(json::value)    → Set JSON body + content-type
    │   ├── .json(json::object)   → Set JSON object
    │   ├── .json(json::array)    → Set JSON array
    │   ├── .text(string_view)    → Set text body + content-type
    │   └── .html(string_view)    → Set HTML body + content-type
    │
    ├── Header Methods (return http_response&)
    │   ├── .header(key, value)   → Add custom header
    │   ├── .status(int)          → Set status code
    │   └── .status(status)       → Set status enum
    │
    └── Special Methods (return http_response&)
        ├── .cookie(...)          → Set cookie with options
        └── .send_file(...)       → Set content-disposition
```

## Method Chaining Flow

```
Constructor
    ↓
wolf::http_response(201)
    ↓
    ├──> .header()  ──┐
    │                 │
    ├──> .status()  ──┤
    │                 ├──> All return http_response&
    ├──> .json()    ──┤    (enables chaining)
    │                 │
    ├──> .text()    ──┤
    │                 │
    ├──> .html()    ──┤
    │                 │
    └──> .cookie()  ──┘
         ↓
    Final Response Object
```

## Usage Patterns

### Pattern 1: Simple Response
```cpp
wolf::http_response(200).json(data)
         ↓              ↓
    Constructor      Content Method
```

### Pattern 2: With Headers
```cpp
wolf::http_response(200).header("X-Key", "val").json(data)
         ↓              ↓                       ↓
    Constructor      Add Header            Set Content
```

### Pattern 3: Complex Chain
```cpp
wolf::http_response(200)
         ↓
    .header("X-API-Version", "1.0")
         ↓
    .header("X-RateLimit", "100")
         ↓
    .json(data)
         ↓
    .cookie("session", "abc", "/", "", 3600, true, false)
         ↓
    Final Response
```

## Request-Response Cycle

```
Client Request
    ↓
HTTP Server (Wolf)
    ↓
Route Matching
    ↓
Handler Function
    ↓
╔════════════════════════════════╗
║   wolf::http_response(200)     ║  ← Constructor
║           ↓                    ║
║   .header("X-Custom", "val")   ║  ← Add headers
║           ↓                    ║
║   .json(data)                  ║  ← Set content
║           ↓                    ║
║   .cookie("name", "val", ...)  ║  ← Set cookie
╚════════════════════════════════╝
    ↓
Response Object
    ↓
Serialization
    ↓
HTTP Response
    ↓
Client
```

## State Transitions

```
Empty State
    ↓
Constructor(status) → Status Set
    ↓
.header() → Headers Added
    ↓
.json() / .text() / .html() → Content + Content-Type Set
    ↓
.cookie() → Cookie Header Added
    ↓
.prepare_payload() → Content-Length Calculated
    ↓
Ready to Send
```

## Memory Model

```cpp
http_response res(200);  // Stack allocated
    ↓
res.json(data);  // Modifies in-place, returns reference
    ↓
res.cookie(...);  // Modifies in-place, returns reference
    ↓
return res;  // Return by value (may use RVO)
```

**Or inline:**

```cpp
return wolf::http_response(200)  // Temporary object
    .json(data)                  // Operates on temporary
    .cookie(...);                // Operates on temporary
                                 // Return by value (NRVO)
```

## Type Safety (C++20 Concepts)

```
User Code
    ↓
Template Instantiation
    ↓
Concept Checking
    ├──> StringLike concept for string parameters
    ├──> StatusType concept for status codes
    └──> RouteString concept for routes
         ↓
Compile-Time Validation
    ├──> Valid → Compilation succeeds
    └──> Invalid → Clear error message
```

## Comparison: Old vs New

### Old Way (Manual)
```
Create response object
    ↓
Set status code
    ↓
Set content-type header
    ↓
Set body content
    ↓
Call prepare_payload()
    ↓
Return response
```

### New Way (Fluent)
```
Constructor with status
    ↓
Chain methods
    ↓
Return (auto prepare_payload)
```

## Performance Characteristics

```
Operation                     Cost
─────────────────────────────────────
Constructor                   O(1)
.header()                     O(1)
.status()                     O(1)
.json()                       O(n) - serialization
.text()                       O(n) - string copy
.html()                       O(n) - string copy
.cookie()                     O(1) + string format
Method chaining              O(1) - return reference
```

## Thread Safety

```
Request Thread 1 ──> http_response(200) ──> Thread-local object
                            ↓
                        Safe to use
                            ↓
                    Return to server
                            ↓
                      Send to client

Request Thread 2 ──> http_response(404) ──> Different object
                            ↓
                        Independent
```

Each request gets its own response object - no shared state.

## Extension Points

```
http_response
    ↓
Custom Methods (future)
    ├──> .xml()        → XML responses
    ├──> .file()       → Stream files
    ├──> .redirect()   → 301/302 redirects
    └──> .cache()      → Cache-Control headers
```

## Integration with Handlers

```cpp
server->get("/api/endpoint", [](const wolf::http_request& req) {
                                        ↑
                                   Request object
    
    auto data = process(req);
           ↓
    Process business logic
           ↓
    return wolf::http_response(200).json(data);
           ↑                                 ↓
    Create response                  Return to server
});
```

## Error Flow

```
Handler Execution
    ↓
Exception thrown?
    ├──> No  → Normal response flow
    │         ↓
    │    return http_response(200).json(data)
    │
    └──> Yes → Catch block
              ↓
         return http_response(500).json(error_obj)
              ↓
         Error response sent to client
```

## Design Principles

1. **Fluent Interface**: Methods return `*this` for chaining
2. **Type Safety**: C++20 concepts prevent misuse
3. **Zero Overhead**: No unnecessary copies (return references)
4. **Immutable State**: Each method call builds on previous state
5. **Composable**: Can be used partially or fully chained
6. **Intuitive**: Reads like natural language
7. **Backward Compatible**: Doesn't break existing code

## Example Data Flow

```
Client: POST /api/users
    ↓
Server receives request
    ↓
Route: /api/users matched
    ↓
Handler invoked with http_request
    ↓
Handler creates wolf::http_response(201)
    ↓
Handler sets JSON: .json(json::object{{"id", 123}})
    ↓
Response object prepared:
    - Status: 201
    - Content-Type: application/json
    - Body: {"id":123}
    ↓
Server serializes response
    ↓
HTTP/1.1 201 Created
Content-Type: application/json
Content-Length: 10

{"id":123}
    ↓
Client receives response
```
