# 🐺 Wolf - Modern C++20 Web Framework

A header-only, high-performance web framework leveraging C++20 features for type safety and expressiveness.

## ✨ Why Wolf?

```cpp
wolf::web_server server(8080);
server->get("/hello", [](auto& req) { return "Hello, World!"; });
server.start();
```

That's it! Three lines to a working web server.

## 🚀 Quick Start (Windows)

### Step 1: Extract Boost

```powershell
# Extract boost.zip (included in project) to project root
# Result: boost/include, boost/lib, boost/bin, boost/cmake
```

### Step 2: Set Environment

```powershell
# Set BOOST_DIR to boost folder location
$env:BOOST_DIR = "$PWD\boost"

# Make it permanent (optional)
[Environment]::SetEnvironmentVariable('BOOST_DIR', "$PWD\boost", 'User')
```

### Step 3: Install Clang (if not installed)

```powershell
winget install LLVM.LLVM

# Verify
clang++ --version
```

### Step 4: Build Examples

```powershell
cd example
.\build.ps1
```

### Step 5: Run

```powershell
cd build
.\example_web.exe

# In another terminal:
curl http://localhost:8080/ping
# Output: pong
```

## 🎯 C++20 Features

Wolf leverages modern C++20 features for safety, performance, and expressiveness:

### ⚡ C++20 Coroutines for Async I/O
```cpp
// Wolf uses coroutines internally for non-blocking request handling
// Handlers execute efficiently without blocking threads
wolf::web_server server(8080);

server->get("/fast", [](const auto& req) {
    // Your handler runs in a coroutine context
    // I/O operations are non-blocking under the hood
    return wolf::http_response(200).json({{"status", "async"}});
});

// Type traits for detecting awaitable types
static_assert(wolf::is_awaitable_v<net::awaitable<int>>);
static_assert(!wolf::is_awaitable_v<int>);
```

**Benefits:**
- 🚀 **Non-blocking I/O**: Requests don't block threads during write operations
- 📈 **Better Scalability**: Handle more concurrent connections with fewer threads
- 🎯 **Clean Code**: Linear async code flow without callback hell
- 🔮 **Future-Ready**: Foundation for async handlers (database queries, external APIs)

See [COROUTINE_IMPLEMENTATION.md](docs/COROUTINE_IMPLEMENTATION.md) for details.

### 🔧 Simplified Router API (NEW!)
```cpp
// Consolidated from 7 overloads to just 2!
// Uses C++20 concepts and if constexpr for compile-time type handling

// All these work automatically:
router.add(GET, "/text", [](auto req) { return "Hello"; });
router.add(GET, "/json", [](auto req) { return http_response(200).json({...}); });
router.add(GET, "/async", [](auto req) -> awaitable<http_response> { co_return ...; });

// Type-safe detection at compile time
template<typename Handler>
concept SyncHandler = !is_awaitable_v<std::invoke_result_t<Handler, request_t>>;

template<typename Handler>
concept AsyncHandler = is_awaitable_v<std::invoke_result_t<Handler, request_t>>;
```

**Benefits:**
- 🎯 **71% Less Code**: 7 overloads reduced to 2 unified methods
- ✅ **Type-Safe**: Compile-time detection via concepts
- 🚀 **Zero Overhead**: All branching resolved at compile-time with `if constexpr`
- 🛡️ **Bug Fixes**: Removed incorrect `co_return *this` statements
- 🧹 **Maintainable**: Single logic path for sync/async handling

### Type-Safe HTTP Methods
```cpp
// C++20: enum class for type safety
using wolf::http_method;

router.add_route(http_method::GET, "/users", handler);
router.add_route(http_method::POST, "/users", handler);
```

### Concepts for Compile-Time Type Checking
```cpp
// RouteString concept ensures only valid route strings
template<RouteString Path>
void route(http_method method, Path&& path, http_handler handler);

// StringLike concept for flexible string handling
template<StringLike S>
http_response make_response(http::status status, S&& body);
```

### std::optional for Safe Value Access
```cpp
// Returns std::optional instead of throwing or returning empty string
auto user_id = req.get("id");  // std::optional<std::string>
if (user_id) {
    // Use *user_id safely
}

// Or use convenience method with default
auto name = req.get_or("name", "Guest");
```

### std::format for String Formatting
```cpp
// Clean, type-safe string formatting
auto response = std::format("User {} has {} posts", user_id, post_count);

// Cookie generation with std::format
set_cookie(res, "session", session_id, "/", 3600);
```

### C++20 Ranges for Data Processing
```cpp
// Cookie parsing using ranges and views
auto cookies = req.cookies();  // Uses std::ranges::split_view

// Clean iteration with structured bindings
for (const auto& [name, value] : cookies) {
    // Process cookie
}
```

### Attributes for Better Code Quality
```cpp
// [[nodiscard]] ensures return values aren't ignored
[[nodiscard]] std::optional<std::string> get(std::string_view key) const;

// [[maybe_unused]] for intentionally unused parameters  
void on_write(beast::error_code ec, [[maybe_unused]] std::size_t bytes);

// constexpr for compile-time evaluation
constexpr std::string_view method_to_string(http_method method);
```

### std::string_view for Zero-Copy Operations
```cpp
// Efficient string handling without allocations
void process_route(std::string_view route);
auto target_clean = request.target();  // Returns string_view
```

## 📖 Examples

### Basic Routes

```cpp
#include "wolf/web_server.hpp"

int main() {
    wolf::web_server server(8080);
    
    // Simple text response
    server->get("/", [](auto& req) {
        return "Welcome to Wolf!";
    });
    
    // Route parameters
    server->get("/users/:id", [](auto& req) {
        return "User ID: " + req.get("id");
    });
    
    // Query parameters  
    server->get("/search", [](auto& req) {
        auto params = req.query_params();
        return "Searching: " + params["q"];
    });
    
    server.start();
}
```

### JSON API

```cpp
#include "wolf/web_server.hpp"
#include <boost/json.hpp>

int main() {
    wolf::web_server server(8080);
    namespace json = boost::json;
    
    // List items
    server->get("/api/items", [](auto& req) {
        return wolf::make_response(
            R"([{"id":1,"name":"Apple"},{"id":2,"name":"Banana"}])",
            http::status::ok,
            "application/json"
        );
    });
    
    // Create item
    server->post("/api/items", [](auto& req) {
        auto data = req.get_json_body();  // Auto-parsed!
        auto obj = data.as_object();
        std::string name = obj["name"].as_string().c_str();
        
        return wolf::make_response(
            R"({"created":true,"name":")" + name + R"("})",
            http::status::created,
            "application/json"
        );
    });
    
    server.start();
}
```

### Cookies

```cpp
// Set cookie
server->get("/login", [](auto& req) {
    wolf::response_t res;
    res.body() = "Logged in!";
    wolf::set_cookie(res, "session", "abc123", "/", "", 3600);  // 1 hour expiry
    return res;
});

// Read cookie
server->get("/dashboard", [](auto& req) {
    auto cookies = req.cookies();
    
    if (cookies.count("session")) {
        return "Welcome back! Session: " + cookies["session"];
    }
    return "Please login first";
});

// Delete cookie (set expiry to 0)
server->get("/logout", [](auto& req) {
    wolf::response_t res;
    res.body() = "Logged out!";
    wolf::set_cookie(res, "session", "", "/", "", 0);
    return res;
});
```

### WebSocket

Wolf automatically handles WebSocket upgrades! No configuration needed.

```cpp
wolf::web_server server(8080);

// Set custom message handler
server->set_socket_handler([](const std::string& msg) {
    return "Echo: " + msg;  // Echo back
});

server.start();

// JavaScript client:
// const ws = new WebSocket('ws://localhost:8080');
// ws.onmessage = (e) => console.log(e.data);
// ws.send('Hello!');
```

## 📚 API Reference

### Request

```cpp
// Get route parameters
auto id = req.get("id");                    // From /users/:id

// Get query parameters
auto params = req.query_params();           // From ?page=1&limit=10
auto page = params["page"];

// Get cookies
auto cookies = req.cookies();
auto session = cookies["session_id"];

// Get JSON body (auto-parsed)
auto json = req.get_json_body();
auto obj = json.as_object();
auto name = obj["name"].as_string().c_str();

// Get raw body
std::string body = req.body();

// Get headers
auto auth = req[http::field::authorization];
```

### Response

Wolf now supports a **modern fluent API** for building responses:

```cpp
// 🆕 NEW: Fluent API (Recommended!)
namespace json = boost::json;

// JSON response with status code
return wolf::http_response(200).json(json::object{
    {"message", "Success"},
    {"data", json::array{1, 2, 3}}
});

// Created response (201)
return wolf::http_response(201).json(json::object{{"id", 123}});

// Error response with headers
return wolf::http_response(404)
    .header("X-Request-ID", "abc123")
    .json(json::object{{"error", "Not Found"}});

// Text response
return wolf::http_response(200).text("Hello, World!");

// HTML response
return wolf::http_response(200).html("<h1>Welcome</h1>");

// Response with cookie
return wolf::http_response(200)
    .json(json::object{{"status", "ok"}})
    .cookie("session", "abc123", "/", "", 3600, true, false);

// Complex chaining
return wolf::http_response(200)
    .header("X-API-Version", "1.0")
    .header("X-RateLimit-Remaining", "99")
    .json(data)
    .cookie("tracking", "xyz", "/", "", 86400, false, false);
```

**Traditional approaches still work:**

```cpp
// Option 1: Return string (simplest)
return "Hello!";  // 200 OK, text/plain

// Option 2: Use helper
return wolf::make_response(
    "Hello!",              // body
    http::status::ok,      // status (default: ok)
    "text/html"            // content-type (default: text/plain)
);

// Option 3: Full control
wolf::response_t res;
res.result(http::status::ok);
res.set(http::field::content_type, "application/json");
res.set(http::field::cache_control, "no-cache");
res.body() = R"({"status":"ok"})";
return res;
```

📖 **[See complete Fluent API documentation →](docs/FLUENT_API.md)**

### Cookies

```cpp
// Set cookie
wolf::set_cookie(
    response,           // response object
    "name",             // cookie name
    "value",            // cookie value
    "/",                // path (default: "/")
    "example.com",      // domain (default: "")
    3600,               // max-age in seconds (default: -1, session)
    true,               // HttpOnly (default: true)
    true                // Secure (default: false)
);

// Read cookie
auto cookies = req.cookies();
if (cookies.count("session")) {
    std::string session = cookies["session"];
}
```

## 🔧 Boost Directory Structure

Your extracted `boost.zip` should look like this:

```
boost/
├── include/
│   └── boost/          # Headers (asio.hpp, beast.hpp, json.hpp, etc.)
├── lib/                # Static libraries (.lib files)
├── bin/                # DLLs (optional, for dynamic linking)
└── cmake/              # CMake config files (optional)
```

If you have Boost elsewhere (e.g., `c:\libraries\boost`), just point `BOOST_DIR` there:

```powershell
$env:BOOST_DIR = "c:\libraries\boost"
```

## 🧪 Build on macOS/Linux

```bash
# macOS
brew install boost llvm
cd example && mkdir build && cd build
cmake .. && make
./example_web

# Linux
sudo apt install libboost-all-dev clang
cd example && mkdir build && cd build  
cmake .. && make
./example_web
```

## ❓ Troubleshooting

### Windows: clang++ not found

```powershell
# Install LLVM
winget install LLVM.LLVM

# Add to PATH if needed
$env:PATH += ";C:\Program Files\LLVM\bin"
```

### Windows: Boost not found

```powershell
# Check BOOST_DIR is set correctly
echo $env:BOOST_DIR

# Should show path to boost folder with include/lib subdirs
dir $env:BOOST_DIR
```

### Port 8080 already in use

```powershell
# Windows: Find and kill process
netstat -ano | findstr :8080
taskkill /PID <PID> /F

# Linux/macOS
lsof -i :8080
kill -9 <PID>
```

## 🆕 What's New in v2.0

### Coroutine-Based Async Architecture
- ✨ **C++20 Coroutines**: Internal request handling now uses `co_await` for non-blocking I/O
- 🔧 **Type Traits**: Added `is_awaitable_v<T>` and `Awaitable` concept for detecting async types
- 📊 **Performance**: Better thread utilization and scalability for concurrent requests
- 🧪 **Tested**: Comprehensive test suite validates async behavior, concurrency, and keep-alive connections

### Simplified Router API ⚡ NEW!
- 🎯 **Reduced Overloads**: From 7 `add()` overloads down to 2 unified methods
- ✅ **Concept-Based**: Uses C++20 concepts for sync/async handler detection
- 🚀 **Compile-Time Branching**: `if constexpr` for zero runtime overhead
- 🛡️ **Bug Fixes**: Removed incorrect `co_return *this` in previous async overloads
- 🧹 **Cleaner Code**: Single implementation for each category (sync/async)

### Enhanced Testing
- ✅ **107 assertions** across coroutine async features
- ✅ Concurrent request handling validated
- ✅ Keep-alive connection tests
- ✅ Error handling verification
- ✅ Type trait compile-time checks

### Documentation
- 📖 [Coroutine Implementation Guide](docs/COROUTINE_IMPLEMENTATION.md) - Architecture and implementation details
- 📊 [Test Coverage Report](docs/COROUTINE_TEST_COVERAGE.md) - Comprehensive test documentation
- 📖 [Fluent API Guide](docs/FLUENT_API.md) - Modern response building patterns
- 📖 [Async Handler Support](src/async_handler_support.hpp) - Future async handler patterns

### Breaking Changes
None! All existing synchronous handlers continue to work. All improvements are backward compatible.

## 🎉 Recent Improvements

### v2.1 - Simplified Router API (Latest)
- ✅ **71% Code Reduction**: 7 `add()` overloads consolidated to 2
- ✅ **Concept-Based Detection**: Compile-time sync/async handler identification
- ✅ **Zero Overhead**: All branching via `if constexpr` (compile-time)
- ✅ **Bug Fixes**: Removed incorrect `co_return *this` statements
- ✅ **All Tests Pass**: 100% backward compatible

### v2.0 - Coroutine Architecture
- ✅ **Non-Blocking I/O**: Request handling via C++20 coroutines
- ✅ **Better Scalability**: More concurrent connections with fewer threads
- ✅ **Type Traits**: `is_awaitable_v<T>` for detecting async types
- ✅ **Fluent API**: Modern response builder with method chaining

## 📝 Project Structure

```
your-project/
├── main.cpp              # Your code
├── wolf/                 # Wolf framework (copy from this repo)
│   ├── http_router.hpp  # Simplified with 2 unified add() overloads!
│   ├── web_server.hpp   # Coroutine-based async I/O
│   └── async_handler_support.hpp  # Future async patterns
└── boost/                # Extracted from boost.zip
    ├── include/
    └── lib/
```

Compile:

```powershell
# Windows
clang++ -std=c++20 -Iboost/include -Iwolf main.cpp -o app.exe -Lboost/lib -lws2_32 -lwsock32

# Linux/macOS  
clang++ -std=c++20 -I/usr/local/include -Iwolf main.cpp -o app -lboost_json -lpthread
```

## 🎯 Requirements

- **C++20** compiler (Clang 14.0+, GCC 11+, MSVC 2022+)
- **Boost 1.81+** with Beast, Asio (with coroutine support), URL, JSON
- **Catch2 v3** (optional, for running tests)

### Compiler Requirements

Wolf requires C++20 features:
- **Coroutines** (`co_await`, `co_return`, `net::awaitable`) ⚡ NEW!
- Concepts and constraints
- `std::format` (or `fmt` library as fallback)
- `std::ranges` and views
- `std::string_view` heterogeneous lookup
- `std::optional` improvements
- Designated initializers
- `[[nodiscard]]`, `[[maybe_unused]]` attributes
- `constexpr` improvements

### Tested Compilers

- ✅ Clang 14.0+ (macOS, Linux, Windows)
- ✅ GCC 11+ (Linux)
- ✅ MSVC 19.30+ / Visual Studio 2022 (Windows)

## 🔄 Migration from Pre-C++20 Version

If upgrading from an older Wolf version, note these breaking changes:

### HTTP Method enum → enum class
```cpp
// Old (C++17)
router.resolve(GET, "/path");

// New (C++20)  
router.resolve(http_method::GET, "/path");
```

### String returns → std::optional
```cpp
// Old (C++17)
std::string value = req.get("key");  // Returns "" if not found

// New (C++20)
auto value = req.get("key");  // Returns std::optional<std::string>
if (value) {
    use(*value);
}

// Or use default value
auto value = req.get_or("key", "default");
```

### boost::tuple → std::tuple
```cpp
// Old (C++17)
boost::tuple<bool, handler, params> result = router.resolve(...);

// New (C++20)
auto [is_trie, handler, params] = router.resolve(...);  // Structured bindings
```

## 🧪 Running Tests (Windows)

Wolf includes comprehensive unit tests using Catch2.

### Setup

1. **Install or build Catch2 v3** and set up the directory structure:

```
c:\libraries\Catch2\
├── include\
│   └── catch2\
│       ├── catch_test_macros.hpp
│       └── ...
└── lib\
    └── Catch2Maind.lib (or Catch2Main.lib)
```

2. **Set environment variables:**

```powershell
$env:BOOST_DIR = "c:\source\wolf\boost"      # Or your Boost location
$env:CATCH2_DIR = "c:\libraries\Catch2"      # Your Catch2 location

# Make permanent
[Environment]::SetEnvironmentVariable('CATCH2_DIR', 'c:\libraries\Catch2', 'User')
```

3. **Build and run tests:**

```powershell
cd tests
.\build.ps1

# Run all tests
cd build
.\router_test.exe
.\web_server_test.exe

# Run specific test
.\router_test.exe "[http_router]"
.\web_server_test.exe "[http_request]"

# List available tests
.\router_test.exe --list-tests
```

### Test Coverage

- **Router Tests** (`router_test.exe`)
  - HTTP method routing (GET, POST, PUT, DELETE, etc.)
  - Trie-based parameterized routes (`/users/:id`)
  - Nested parameters (`/api/:version/users/:userId`)
  - Static and dynamic route coexistence
  - Edge cases (trailing slashes, empty routes)

- **Web Server Tests** (`web_server_test.exe`) ⚡ Updated!
  - HTTP request/response handling
  - Cookie parsing (single, multiple, with whitespace)
  - Cookie setting (with path, domain, max-age, secure, httponly)
  - Query parameter extraction
  - Status code validation
  - RESTful API patterns
  - **🆕 Coroutine Async Features**:
    - Awaitable type trait detection (`is_awaitable_v`)
    - Async request handling with real server/client
    - Multiple concurrent requests (5+ simultaneous)
    - Keep-alive connection handling
    - Error handling in coroutine context
    - Type trait compile-time validation

**Test Results**: ✅ 107 assertions passed across 7 test cases

See `tests/README.md` and [COROUTINE_TEST_COVERAGE.md](docs/COROUTINE_TEST_COVERAGE.md) for detailed test documentation.

## 📖 More Examples

See `example/` folder:
- `example_router.cpp` - Routing examples
- `example_web.cpp` - Full web server with cookies
- `test_client.cpp` - HTTP client
- `websocket_test.html` - Browser WebSocket test
- 🆕 `async_handler_example.cpp` - Coroutine patterns and future async handler usage

## 📚 Documentation

- [Coroutine Implementation](docs/COROUTINE_IMPLEMENTATION.md) - Architecture, benefits, and migration guide
- [Test Coverage Report](docs/COROUTINE_TEST_COVERAGE.md) - Comprehensive test documentation
- [Fluent API Guide](docs/FLUENT_API.md) - Modern response building patterns
- [Async Handler Support](src/async_handler_support.hpp) - Future async patterns (reference implementation)

## 🙏 Credits

Built with [Boost.Beast](https://www.boost.org/doc/libs/release/libs/beast/), [Boost.Asio](https://www.boost.org/doc/libs/release/libs/asio/) with coroutine support, and [Boost.JSON](https://www.boost.org/doc/libs/release/libs/json/).

---

**Made with ❤️ using modern C++20 coroutines**
