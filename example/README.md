# Wolf Framework - Examples

This directory contains example applications demonstrating the Wolf web server framework features.

## Examples

### 1. Router Example (`example_router`)
Demonstrates the HTTP router functionality without running a server.

**Features:**
- Static route registration
- Parameterized routes (`:id`, `:userId`, etc.)
- Route resolution and parameter extraction
- Handler functions
- Error handling for non-existent routes

**Usage:**
```bash
cd build/example
./example_router
```

**Output:**
```
GET /
  Route type: Static
  Response: Hello, World! (Status: 200)

GET /users/123
  Route type: Parameterized
  Parameters: id=123 
  Response: User details for: ID=123 (Status: 200)

GET /users/456/posts/789
  Route type: Parameterized
  Parameters: id=456 postId=789 
  Response: Posts for user: PostID=789 (Status: 200)
```

### 2. Web Server Example (`example_web`)
Demonstrates a complete HTTP web server with Boost.Beast.

**Features:**
- HTTP server on port 8080
- Route handlers
- Request/response handling
- Async I/O with Boost.Asio

**Usage:**
```bash
cd build/example
./example_web
```

Then test with curl:
```bash
curl http://localhost:8080/
# Output: Hello, World!
```

## Building Examples

### Option 1: Build all examples
```bash
cd wolf
mkdir -p build
cd build
cmake ..
make example_router example_web
```

### Option 2: Build specific example
```bash
cd wolf
mkdir -p build
cd build
cmake ..
make example_router
# or
make example_web
```

### Option 3: Build examples separately
```bash
cd wolf/example
mkdir -p build
cd build
cmake ..
make
```

## Adding Your Own Example

1. Create a new `.cpp` file in the `example/` directory
2. Update `example/CMakeLists.txt`:

```cmake
# Add your example
add_executable(my_example my_example.cpp)
target_link_libraries(my_example 
    PRIVATE 
    ${Boost_LIBRARIES}
)

# Add compiler options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(my_example PRIVATE -Wall -Wextra -Wpedantic)
endif()
```

3. Rebuild:
```bash
cd build
cmake ..
make my_example
```

## Example Code Snippets

### Basic Router Setup
```cpp
#include "http_router.hpp"

wolf::http_router<Response, Request> router;

router
    .get("/", hello_handler)
    .get("/users/:id", user_handler)
    .post("/users", create_user_handler);

auto [is_trie, handler, params] = router.resolve(GET, "/users/123");
if (handler) {
    Response res = handler(request);
}
```

### Web Server Setup
```cpp
#include "web_server.hpp"

wolf::web_server app(8080);

app->get("/", [](const wolf::http_request& req) {
    wolf::response_t res;
    res.result(http::status::ok);
    res.body() = "Hello, World!";
    return res;
});

app.start();
```

### Parameterized Routes
```cpp
app->get("/users/:id", [](const wolf::http_request& req) {
    // Access URI parameters through the request
    wolf::response_t res;
    res.result(http::status::ok);
    res.body() = "User ID: " + std::string(req.target());
    return res;
});
```

### RESTful API
```cpp
// List all users
app->get("/api/users", list_users_handler);

// Get specific user
app->get("/api/users/:id", get_user_handler);

// Create new user
app->post("/api/users", create_user_handler);

// Update user
app->put("/api/users/:id", update_user_handler);

// Delete user
app->del("/api/users/:id", delete_user_handler);
```

## Requirements

- C++23 compatible compiler
- Boost libraries (json, url, asio, beast)
- CMake 3.16 or higher

## Notes

- Examples use `EXCLUDE_FROM_ALL` in the main CMakeLists.txt, so they won't build automatically with `make`
- Build examples explicitly with `make example_router` or `make example_web`
- The web server example runs on port 8080 by default
- Press Ctrl+C to stop the web server

## Troubleshooting

**Port already in use:**
```
Error: Address already in use
```
Solution: Either stop the process using port 8080 or change the port in the code.

**Boost libraries not found:**
```
CMake Error: Could not find Boost
```
Solution: Install Boost libraries:
```bash
# macOS
brew install boost

# Ubuntu/Debian
sudo apt-get install libboost-all-dev
```

**Compiler errors:**
Make sure you're using a C++23 compatible compiler:
```bash
# Check compiler version
g++ --version
clang++ --version
```

## Next Steps

- Check out the [tests](../tests/) directory for more usage examples
- Read the main project documentation
- Explore the [web_server.hpp](../web_server.hpp) and [http_router.hpp](../http_router.hpp) headers
