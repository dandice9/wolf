# 🐺 Wolf Web Framework

A modern, header-only C++23 web framework with high-performance trie-based routing and asynchronous HTTP/WebSocket support built on Boost.Beast.

## ✨ Features

- 🚀 **Header-only** - Just include and use
- 🔥 **Fast Routing** - Trie-based algorithm with O(k) complexity
- 🎯 **Dynamic Parameters** - Extract route parameters (e.g., `/users/:id`)
- ⚡ **Async I/O** - Non-blocking with Boost.Asio
- 🌐 **HTTP/1.1** - Full HTTP support via Boost.Beast
- 🔌 **WebSocket** - Built-in WebSocket upgrade capability
- 🧵 **Multi-threaded** - Auto-scaled thread pool
- 🎨 **Modern C++23** - Type-safe and expressive
- ✅ **Tested** - 152 unit test assertions (100% pass rate)

## 📋 Requirements

### Compiler Support
- **C++23** standard required
- GCC 12+ / Clang 16+ / MSVC 19.30+ (Visual Studio 2022+)

### Dependencies
- **Boost 1.89.0+** (components: json, url, system, asio, beast)
- **CMake 4.1+** (for building)
- **Catch2 v3** (for tests, optional)

## 🚀 Quick Start

### Simple Example

```cpp
#include "wolf/web_server.hpp"
#include <iostream>

using namespace wolf;

int main() {
    // Create server on port 8080
    web_server server(8080);
    
    // Simple GET route
    server->get("/", [](const http_request& req) {
        response_t response;
        response.result(http::status::ok);
        response.set(http::field::content_type, "text/html");
        response.body() = "<h1>Hello, Wolf!</h1>";
        return response;
    });
    
    // JSON API endpoint
    server->get("/api/hello", [](const http_request& req) {
        response_t response;
        response.result(http::status::ok);
        response.set(http::field::content_type, "application/json");
        response.body() = R"({"message": "Hello from Wolf!"})";
        return response;
    });
    
    // Route with parameters
    server->get("/users/:id", [](const http_request& req) {
        auto params = req.params();
        std::string user_id = params["id"];
        
        response_t response;
        response.result(http::status::ok);
        response.set(http::field::content_type, "application/json");
        response.body() = R"({"user_id": ")" + user_id + R"("})";
        return response;
    });
    
    std::cout << "Server running on http://localhost:8080" << std::endl;
    server.start();  // Blocking call
    
    return 0;
}
```

### REST API Example

```cpp
#include "wolf/web_server.hpp"
#include <boost/json.hpp>

using namespace wolf;
namespace json = boost::json;

int main() {
    web_server server(8080);
    
    // GET - List all items
    server->get("/api/items", [](const http_request& req) {
        json::array items = {
            {{"id", 1}, {"name", "Item 1"}},
            {{"id", 2}, {"name", "Item 2"}}
        };
        
        response_t response;
        response.result(http::status::ok);
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(items);
        return response;
    });
    
    // GET - Get item by ID
    server->get("/api/items/:id", [](const http_request& req) {
        auto id = req.params()["id"];
        
        json::object item = {
            {"id", std::stoi(id)},
            {"name", "Item " + id}
        };
        
        response_t response;
        response.result(http::status::ok);
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(item);
        return response;
    });
    
    // POST - Create item
    server->post("/api/items", [](const http_request& req) {
        // Parse request body
        auto body = json::parse(req.body());
        
        json::object result = {
            {"id", 3},
            {"created", true},
            {"data", body}
        };
        
        response_t response;
        response.result(http::status::created);  // 201
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(result);
        return response;
    });
    
    // PUT - Update item
    server->put("/api/items/:id", [](const http_request& req) {
        auto id = req.params()["id"];
        auto body = json::parse(req.body());
        
        json::object result = {
            {"id", std::stoi(id)},
            {"updated", true},
            {"data", body}
        };
        
        response_t response;
        response.result(http::status::ok);
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(result);
        return response;
    });
    
    // PATCH - Partial update
    server->patch("/api/items/:id", [](const http_request& req) {
        auto id = req.params()["id"];
        auto body = json::parse(req.body());
        
        json::object result = {
            {"id", std::stoi(id)},
            {"patched", true},
            {"data", body}
        };
        
        response_t response;
        response.result(http::status::ok);
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(result);
        return response;
    });
    
    // DELETE - Delete item
    server->del("/api/items/:id", [](const http_request& req) {
        auto id = req.params()["id"];
        
        json::object result = {
            {"id", std::stoi(id)},
            {"deleted", true}
        };
        
        response_t response;
        response.result(http::status::ok);
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(result);
        return response;
    });
    
    std::cout << "REST API running on http://localhost:8080/api" << std::endl;
    server.start();
    
    return 0;
}
```

## 🛠️ Installation & Build

### Prerequisites

#### Install Boost

**macOS:**
```bash
brew install boost
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install libboost-all-dev
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf install boost-devel
```

**Windows (vcpkg):**
```powershell
vcpkg install boost-json boost-url boost-asio boost-beast
```

**Windows (Manual):**
- Download Boost from https://www.boost.org/users/download/
- Extract and set `BOOST_ROOT` environment variable

#### Install Catch2 (Optional, for tests)

**macOS:**
```bash
brew install catch2
```

**Linux:**
```bash
# Ubuntu/Debian
sudo apt install catch2

# Or build from source
git clone https://github.com/catchorg/Catch2.git
cd Catch2
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

**Windows (vcpkg):**
```powershell
vcpkg install catch2
```

### Building Your Project

#### Method 1: Header-Only (Simplest)

Just include the headers and compile:

**macOS/Linux:**
```bash
g++ -std=c++23 -I./wolf -I/usr/local/include \
    your_app.cpp -o your_app \
    -L/usr/local/lib -lboost_system -lboost_json -lpthread

./your_app
```

**Windows (MSVC):**
```powershell
cl /std:c++latest /EHsc /I.\wolf /I"C:\boost\include" ^
   your_app.cpp /Fe:your_app.exe ^
   /link /LIBPATH:"C:\boost\lib"

your_app.exe
```

#### Method 2: Using CMake (Recommended)

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyWolfApp CXX)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost
find_package(Boost 1.89.0 REQUIRED COMPONENTS system json)

# Add your executable
add_executable(my_app src/main.cpp)

# Include wolf headers
target_include_directories(my_app PRIVATE 
    ${CMAKE_SOURCE_DIR}/wolf
    ${Boost_INCLUDE_DIRS}
)

# Link Boost libraries
target_link_libraries(my_app PRIVATE 
    Boost::system 
    Boost::json
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(my_app PRIVATE ws2_32 wsock32)
elseif(UNIX)
    target_link_libraries(my_app PRIVATE pthread)
endif()
```

**Build on macOS/Linux:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
./my_app
```

**Build on Windows:**
```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
.\Release\my_app.exe
```

**Cross-platform build:**
```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run
./build/my_app                    # macOS/Linux
.\build\Release\my_app.exe        # Windows
```

## 📁 Project Structure

```
your_project/
├── CMakeLists.txt           # Build configuration
├── src/
│   └── main.cpp            # Your application code
└── wolf/                   # Wolf framework (copy these files)
    ├── http_router.hpp     # Routing engine
    └── web_server.hpp      # Web server
```

## 🎯 Complete Example Project

### Directory Structure
```
my_web_app/
├── CMakeLists.txt
├── src/
│   └── main.cpp
└── wolf/
    ├── http_router.hpp
    └── web_server.hpp
```

### `CMakeLists.txt`
```cmake
cmake_minimum_required(VERSION 3.16)
project(MyWebApp CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.89.0 REQUIRED COMPONENTS system json)

add_executable(my_web_app src/main.cpp)

target_include_directories(my_web_app PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(my_web_app PRIVATE 
    Boost::system 
    Boost::json
)

if(WIN32)
    target_link_libraries(my_web_app PRIVATE ws2_32 wsock32)
elseif(UNIX)
    target_link_libraries(my_web_app PRIVATE pthread)
endif()
```

### `src/main.cpp`
```cpp
#include "wolf/web_server.hpp"
#include <boost/json.hpp>
#include <iostream>

using namespace wolf;
namespace json = boost::json;

int main() {
    web_server server(8080);
    
    // Home page
    server->get("/", [](const http_request& req) {
        response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/html");
        res.body() = R"(
            <!DOCTYPE html>
            <html>
            <head><title>My Wolf App</title></head>
            <body>
                <h1>Welcome to My Wolf App!</h1>
                <p><a href="/api/users">View Users</a></p>
            </body>
            </html>
        )";
        return res;
    });
    
    // API endpoint
    server->get("/api/users", [](const http_request& req) {
        json::array users = {
            {{"id", 1}, {"name", "Alice"}},
            {{"id", 2}, {"name", "Bob"}}
        };
        
        response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = json::serialize(users);
        return res;
    });
    
    // User by ID
    server->get("/api/users/:id", [](const http_request& req) {
        auto id = req.params()["id"];
        
        json::object user = {
            {"id", std::stoi(id)},
            {"name", "User " + id}
        };
        
        response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = json::serialize(user);
        return res;
    });
    
    std::cout << "🐺 Server running at http://localhost:8080\n";
    std::cout << "Press Ctrl+C to stop\n";
    
    server.start();
    return 0;
}
```

### Build & Run

**macOS/Linux:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./my_web_app
```

**Windows (PowerShell):**
```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
.\Release\my_web_app.exe
```

**Test:**
```bash
# In another terminal
curl http://localhost:8080/
curl http://localhost:8080/api/users
curl http://localhost:8080/api/users/1
```

## 📚 API Reference

### Web Server

```cpp
// Create server
web_server server(port);

// Define routes
server->get(path, handler);
server->post(path, handler);
server->put(path, handler);
server->patch(path, handler);
server->del(path, handler);  // DELETE

// Start server (blocking)
server.start();
```

### HTTP Request

```cpp
http_request& req;

// Get route parameters
auto params = req.params();  // boost::unordered_map<string, string>
std::string id = params["id"];

// Get query parameters
auto query = req.query();
std::string search = query["q"];

// Get request body
std::string body = req.body();

// Get headers
std::string content_type = req[http::field::content_type];
```

### HTTP Response

```cpp
response_t res;

// Set status code
res.result(http::status::ok);           // 200
res.result(http::status::created);      // 201
res.result(http::status::not_found);    // 404

// Set headers
res.set(http::field::content_type, "application/json");
res.set(http::field::server, "MyApp");

// Set body
res.body() = "Hello, World!";
res.body() = json::serialize(data);

return res;
```

### Route Patterns

```cpp
// Static routes
server->get("/about", handler);

// Dynamic parameters
server->get("/users/:id", handler);
server->get("/posts/:postId/comments/:commentId", handler);

// Parameters are extracted automatically
auto id = req.params()["id"];
auto postId = req.params()["postId"];
```

## 🧪 Running Tests

```bash
cd build
cmake .. -DBUILD_TESTING=ON
cmake --build .

# Run all tests
ctest

# Or run individually
./tests/router_tests
./tests/web_server_tests
```

## 🔧 Troubleshooting

### Boost Not Found

**macOS/Linux:**
```bash
# Set BOOST_ROOT environment variable
export BOOST_ROOT=/usr/local/opt/boost  # macOS Homebrew
export BOOST_ROOT=/usr/include/boost    # Linux

# Then rebuild
cmake -B build -DBOOST_ROOT=$BOOST_ROOT
```

**Windows:**
```powershell
# Set environment variable
$env:BOOST_ROOT="C:\boost"

# Or specify in CMake
cmake -B build -DBOOST_ROOT="C:\boost"
```

### C++23 Not Supported

Make sure you have a recent compiler:
- GCC 12+ (Linux: `gcc --version`)
- Clang 16+ (macOS: `clang --version`)
- MSVC 19.30+ (Visual Studio 2022)

### Link Errors on Windows

Add to `CMakeLists.txt`:
```cmake
if(WIN32)
    target_link_libraries(my_app PRIVATE ws2_32 wsock32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()
```

### Port Already in Use

```bash
# Linux/macOS - Find and kill process
lsof -i :8080
kill -9 <PID>

# Windows - Find and kill process
netstat -ano | findstr :8080
taskkill /PID <PID> /F
```

## 📖 Examples

See the `example/` directory for more complete examples:
- `example_router.cpp` - Routing examples
- `example_web.cpp` - Full REST API server
- `test_client.cpp` - HTTP client for testing

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## 📄 License

This project is open source. See LICENSE file for details.

## 🙏 Acknowledgments

Built with:
- [Boost.Beast](https://www.boost.org/doc/libs/release/libs/beast/) - HTTP and WebSocket
- [Boost.Asio](https://www.boost.org/doc/libs/release/libs/asio/) - Async I/O
- [Boost.JSON](https://www.boost.org/doc/libs/release/libs/json/) - JSON support
- [Catch2](https://github.com/catchorg/Catch2) - Testing framework

---

Made with ❤️ using modern C++23
