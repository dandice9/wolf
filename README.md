# 🐺 Wolf - Modern C++20 Web Framework

A header-only, high-performance web framework leveraging C++20 features for type safety and expressiveness.

## ✨ Why Wolf?

```cpp
wolf::web_server server(8080);

// ✅ Synchronous handler
server->get("/hello", [](auto& req) { 
    return "Hello, World!"; 
});

// ✅ Asynchronous handler - just add the return type!
server->get("/async", [](auto& req) -> net::awaitable<wolf::http_response> {
    co_return wolf::http_response(200).text("Async response");
});

server.start();
```

That's it! Mix sync and async handlers freely in the same router - no configuration needed.

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

### ⚡ Unified Router - Mix Sync & Async Handlers
```cpp
wolf::web_server server(8080);

// ✅ Synchronous handler - returns http_response directly
server->get("/sync", [](const auto& req) {
    return wolf::http_response(200).text("Sync response");
});

// ✅ Asynchronous handler - returns awaitable<http_response>
server->get("/async", [](const auto& req) -> net::awaitable<wolf::http_response> {
    // Simulate async work (database query, external API, etc.)
    auto result = co_await some_async_operation();
    co_return wolf::http_response(200).json(result);
});

// ✅ Mix them freely in the same router!
server->get("/users/:id", [](const auto& req) -> net::awaitable<wolf::http_response> {
    auto id = req.params().at("id");
    auto user = co_await database.find_user(id);
    co_return wolf::http_response(200).json(user);
});
```

**Benefits:**
- 🎯 **No Upfront Choice**: Don't decide "sync router" vs "async router" - just use `wolf_router`
- � **Automatic Detection**: Handler type detected at compile-time via type traits
- 🚀 **Zero Overhead**: Sync handlers execute directly, async handlers use coroutines
- 🛡️ **Type-Safe**: Compile-time validation using C++20 concepts
- 🧩 **Mix Freely**: Use sync for simple operations, async for I/O - all in one router

**Type Traits:**
```cpp
// Compile-time detection of awaitable types
static_assert(wolf::is_awaitable_v<net::awaitable<int>>);
static_assert(!wolf::is_awaitable_v<int>);

// Concept-based handler detection
template<typename Handler>
concept SyncHandler = !is_awaitable_v<std::invoke_result_t<Handler, http_request>>;
```

See [UNIFIED_ROUTER.md](docs/UNIFIED_ROUTER.md) for complete guide.

### 🔧 Type Erasure with std::variant
```cpp
// Unified handler using std::variant for safe storage
template<typename PT>
class unified_handler {
    using sync_fn = std::function<response_t(PT)>;
    using async_fn = std::function<net::awaitable<response_t>(PT)>;
    using handler_variant = std::variant<std::monostate, sync_fn, async_fn>;
    
    handler_variant handler_;  // Only one handler stored at a time!
    
public:
    // Accepts both sync and async handlers
    unified_handler(sync_fn fn) : handler_(std::move(fn)) {}
    unified_handler(async_fn fn) : handler_(std::move(fn)) {}
    
    // Always returns awaitable (sync handlers wrapped automatically)
    [[nodiscard]] net::awaitable<response_t> operator()(PT req) const {
        if (std::holds_alternative<async_fn>(handler_)) {
            co_return co_await std::get<async_fn>(handler_)(req);
        } else {
            co_return std::get<sync_fn>(handler_)(req);
        }
    }
};
```

**Benefits:**
- 🛡️ **Memory Safe**: No empty `std::function` objects that could cause segfaults
- 📦 **Space Efficient**: Only stores the active handler (not both)
- ✅ **Type Safe**: `std::variant` provides compile-time type safety
- 🎯 **Clean API**: Single router type for all handler types

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
        auto id = req.params().at("id");
        return "User ID: " + id;
    });
    
    // Query parameters  
    server->get("/search", [](auto& req) {
        auto params = req.query_params();
        return "Searching: " + params["q"];
    });
    
    server.start();
}
```

### Unified Router - Mix Sync & Async

```cpp
#include "wolf/web_server.hpp"
#include <boost/asio.hpp>

namespace net = boost::asio;

int main() {
    wolf::web_server server(8080);
    
    // ✅ Synchronous handler - returns http_response directly
    server->get("/health", [](const auto& req) {
        return wolf::http_response(200).text("OK");
    });
    
    // ✅ Asynchronous handler - returns awaitable<http_response>
    server->get("/db-query", [](const auto& req) -> net::awaitable<wolf::http_response> {
        // Simulate async database query
        auto conn = co_await db_pool.acquire();
        auto users = co_await conn.query("SELECT * FROM users");
        co_return wolf::http_response(200).json(users);
    });
    
    // ✅ Sync with route parameters
    server->get("/cache/:key", [&cache](const auto& req) {
        auto key = req.params().at("key");
        auto value = cache.get(key);
        return wolf::http_response(200).json({{"value", value}});
    });
    
    // ✅ Async with route parameters
    server->get("/users/:id", [](const auto& req) -> net::awaitable<wolf::http_response> {
        auto id = req.params().at("id");
        auto user = co_await database.find_user(id);
        co_return wolf::http_response(200).json(user);
    });
    
    // ✅ Async POST handler
    server->post("/api/data", [](const auto& req) -> net::awaitable<wolf::http_response> {
        auto body = req.get_json_body();
        auto result = co_await api_client.post("https://external-api.com", body);
        co_return wolf::http_response(201).json(result);
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

## 🆕 What's New in v2.2

### 🎯 Unified Router - The Ultimate Solution
**No more choosing between sync and async routers!**

```cpp
// Before: Had to choose upfront
wolf_router sync_router;          // Only for sync handlers
wolf_async_router async_router;   // Only for async handlers

// Now: Single unified router accepts both!
wolf_router router;  // That's it!

// Mix freely
router.get("/sync", [](auto& req) { return "Hello"; });
router.get("/async", [](auto& req) -> net::awaitable<http_response> {
    co_return http_response(200).text("Async");
});
```

**Key Features:**
- ✨ **Type Erasure with `std::variant`**: Safe storage of either sync or async handlers
- 🛡️ **Memory Safe**: Fixed segfault issue from empty `std::function` calls
- 🎯 **Automatic Detection**: Handler type detected at compile-time
- 🚀 **Zero Overhead**: Sync handlers execute directly (no coroutine overhead)
- 🔄 **Seamless Wrapping**: Sync handlers automatically wrapped when needed

**Implementation Highlights:**
```cpp
// Unified handler using std::variant for type-safe storage
template<typename PT>
class unified_handler {
    std::variant<std::monostate, sync_fn, async_fn> handler_;
    
    // Always returns awaitable (wraps sync handlers automatically)
    net::awaitable<response_t> operator()(PT req) const;
};
```

### Enhanced Testing
- ✅ **379 assertions** passing across all test suites
- ✅ Both sync and async handlers validated
- ✅ Concurrent request handling
- ✅ Memory safety verified (no segfaults)
- ✅ Route parameters with async handlers

### Documentation
- 📖 [Unified Router Guide](docs/UNIFIED_ROUTER.md) - Complete migration and usage guide
- 📖 [Fluent API Guide](docs/FLUENT_API.md) - Modern response building patterns
- 📊 [Test Coverage Report](docs/COROUTINE_TEST_COVERAGE.md) - Comprehensive test documentation

### Previous Updates (v2.0 - v2.1)
- ✨ **C++20 Coroutines**: Internal request handling uses `co_await` for non-blocking I/O
- 🔬 **Type Traits**: Added `is_awaitable_v<T>` and `Awaitable` concept
- 📊 **Performance**: Better thread utilization and scalability
- 🎯 **Progressive Simplification**: 7 overloads → 2 overloads → 1 unified function
- ✅ **Concept-Based**: Uses C++20 concepts for handler detection

### Breaking Changes
None! All existing synchronous handlers continue to work. All improvements are backward compatible.

## 🎉 Recent Improvements

### v2.3 - Code Simplification & IntelliSense Enhancements (December 2024) 🎯
- ✅ **Merged `add()` Functions**: Consolidated two separate `add()` functions into one using `if constexpr`
  - Eliminated code duplication between sync and async handler paths
  - Single function signature with compile-time branching
  - Cleaner codebase with unified logic flow
- ✅ **Improved IntelliSense Support**: Enhanced type resolution for better IDE experience
  - Added explicit type aliases (`middleware_t`, `middleware_list_t`) to reduce abstraction
  - Replaced `auto` with concrete types where IntelliSense struggled with templates
  - Better code completion and navigation in VS Code
- ✅ **Maintained Zero Overhead**: All improvements use compile-time features
  - `if constexpr` ensures no runtime branching
  - Type aliases have zero runtime cost
  - Full backward compatibility preserved

**Code Example - Unified `add()` function:**
```cpp
// Single function handles both sync and async with compile-time detection
template<RouteString T, typename Handler>
requires std::invocable<Handler, PT>
http_router& add(http_method method, T&& route, Handler&& handler) {
    constexpr bool is_async_handler = is_awaitable_v<return_t>;
    
    if constexpr (is_async_handler) {
        // Async handler path
    } else {
        // Sync handler path
    }
}
```

### v2.2 - Unified Router 🎯
- ✅ **Single Router Type**: No more choosing between `wolf_router` and `wolf_async_router`
- ✅ **Type Erasure**: Safe handler storage using `std::variant`
- ✅ **Memory Safety**: Fixed segfault from empty `std::function` calls
- ✅ **Mix Freely**: Sync and async handlers in the same router
- ✅ **379 Tests Passing**: Comprehensive validation of unified approach
- ✅ **Example Working**: Full demo with sync, async, POST, and parameterized routes

### v2.1 - Simplified Router API
- ✅ **Code Reduction**: From 7 `add()` overloads to 2, now merged into 1
- ✅ **Concept-Based Detection**: Compile-time sync/async handler identification
- ✅ **Zero Overhead**: All branching via `if constexpr` (compile-time)

### v2.0 - Coroutine Architecture
- ✅ **Non-Blocking I/O**: Request handling via C++20 coroutines
- ✅ **Better Scalability**: More concurrent connections with fewer threads
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

## � More Examples

See `examples/` folder:
- `example_router.cpp` - Routing examples
- `example_web.cpp` - Full web server with cookies
- `test_client.cpp` - HTTP client
- `websocket_test.html` - Browser WebSocket test
- 🆕 `unified_router_example.cpp` - **Mix sync & async handlers in one router**

## 📚 Documentation

- 🆕 [Unified Router Guide](docs/UNIFIED_ROUTER.md) - **Complete guide to mixing sync & async handlers**
- [Fluent API Guide](docs/FLUENT_API.md) - Modern response building patterns
- [Test Coverage Report](docs/COROUTINE_TEST_COVERAGE.md) - Comprehensive test documentation
- [Coroutine Implementation](docs/COROUTINE_IMPLEMENTATION.md) - Architecture and internals

## 🎓 Quick Reference

### Handler Types

```cpp
// ✅ Synchronous - returns http_response directly
server->get("/sync", [](const auto& req) {
    return wolf::http_response(200).text("Hello");
});

// ✅ Asynchronous - returns awaitable<http_response>
// Must annotate return type: -> net::awaitable<wolf::http_response>
server->get("/async", [](const auto& req) -> net::awaitable<wolf::http_response> {
    auto result = co_await async_operation();
    co_return wolf::http_response(200).json(result);
});
```

### When to Use Each

| Use Case | Handler Type | Example |
|----------|--------------|---------|
| Simple calculation | Sync | `return http_response(200).text(std::to_string(2 + 2));` |
| In-memory lookup | Sync | `return http_response(200).json(cache.get(key));` |
| Database query | Async | `auto data = co_await db.query("SELECT..."); co_return ...;` |
| External API call | Async | `auto result = co_await http_client.get(url); co_return ...;` |
| File I/O | Async | `auto content = co_await file.read(); co_return ...;` |

### Common Pitfalls

```cpp
// ❌ WRONG - Forgot return type annotation
server->get("/async", [](const auto& req) {
    co_await something();  // ERROR: can't deduce return type
    co_return http_response(200);
});

// ✅ CORRECT - Always annotate async handlers
server->get("/async", [](const auto& req) -> net::awaitable<wolf::http_response> {
    co_await something();
    co_return http_response(200);
});

// ❌ WRONG - Blocking I/O in sync handler
server->get("/slow", [](const auto& req) {
    auto data = blocking_db_query();  // Blocks entire thread!
    return http_response(200).json(data);
});

// ✅ CORRECT - Use async for I/O
server->get("/fast", [](const auto& req) -> net::awaitable<wolf::http_response> {
    auto data = co_await async_db_query();  // Non-blocking
    co_return http_response(200).json(data);
});
```

## 🙏 Credits

Built with [Boost.Beast](https://www.boost.org/doc/libs/release/libs/beast/), [Boost.Asio](https://www.boost.org/doc/libs/release/libs/asio/) with coroutine support, and [Boost.JSON](https://www.boost.org/doc/libs/release/libs/json/).

---

**Made with ❤️ using modern C++20 coroutines and type erasure**
