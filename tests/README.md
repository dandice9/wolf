# Wolf Web Server - Test Suite

This directory contains comprehensive test suites for the Wolf web server framework using Catch2.

## Test Executables

### 1. Router Tests (`router_tests`)
Tests for the HTTP router and trie-based routing system.

**Test Cases:**
- **HTTP Router - Basic functionality** `[http_router]`
  - Add and resolve simple routes
  - Method chaining
  - All HTTP methods (GET, POST, PUT, DELETE, PATCH, OPTIONS, HEAD, CONNECT, TRACE)
  - Route not found handling

- **Trie Router - Parameter routes** `[trie_router]`
  - Single parameter routes (`/users/:id`)
  - Multiple parameter routes (`/users/:userId/posts/:postId`)
  - Mixed static and parameter routes
  - Route detection

- **Trie Router - Direct trie testing** `[trie_router]`
  - Insert and search operations
  - Non-existent route handling
  - Complex nested routes
  - Partial route match validation

- **HTTP Methods - Enum and string conversion** `[http_methods]`
  - Method to string conversion for all HTTP verbs

- **Integration - Mixed route types** `[integration]`
  - Coexistence of static and parameterized routes
  - Route type detection and parameter extraction

- **Edge cases** `[edge_cases]`
  - Empty routes
  - Routes with trailing slashes
  - Multiple slashes handling

**Statistics:** 93 assertions across 6 test cases

### 2. Web Server Tests (`web_server_tests`)
Tests for the web server components, HTTP request/response handling, and wolf router integration.

**Test Cases:**
- **HTTP Request - Parameter handling** `[http_request]`
  - URI parameters
  - Query parameters
  - Empty parameters

- **Wolf Router - HTTP method mapping** `[wolf_router]`
  - Basic route registration
  - Parameterized routes
  - Multiple HTTP methods on same path
  - Route not found returns nullptr

- **Wolf Router - Complex routing scenarios** `[wolf_router]`
  - Nested parameterized routes
  - Mixed static and dynamic routes
  - RESTful API patterns (full CRUD operations)

- **Response building** `[response]`
  - Basic responses
  - Error responses
  - Redirect responses

- **HTTP Status Codes** `[status_codes]`
  - Success codes (200, 201, 202, 204)
  - Client error codes (400, 401, 403, 404)
  - Server error codes (500, 501, 502, 503)

**Statistics:** 59 assertions across 5 test cases

## Building and Running Tests

### Prerequisites
- CMake 3.16 or higher
- C++23 compatible compiler
- Boost libraries (json, url)
- Catch2 v3

### Build Instructions

```bash
# Create build directory
cd wolf
mkdir -p build
cd build

# Configure and build
cmake ..
make

# Run all tests
ctest --verbose

# Or run individual test suites
./router_tests
./web_server_tests
```

### Running Specific Tests

```bash
# Run only router tests
./router_tests

# Run only trie router tests
./router_tests "[trie_router]"

# Run with detailed output
./router_tests -s

# List all available tests
./router_tests --list-tests

# Run only web server tests
./web_server_tests

# Run only wolf router tests
./web_server_tests "[wolf_router]"
```

## Test Coverage

### Router Module
- ✅ Static route resolution
- ✅ Parameterized route resolution
- ✅ Trie-based routing algorithm
- ✅ Parameter extraction
- ✅ HTTP method handling
- ✅ Route not found handling
- ✅ Method chaining (fluent interface)
- ✅ Edge cases

### Web Server Module
- ✅ HTTP request construction
- ✅ URI and query parameter handling
- ✅ Wolf router integration
- ✅ Response building
- ✅ HTTP status codes
- ✅ RESTful API patterns
- ✅ Multiple HTTP methods
- ✅ Error handling

## Key Features Tested

1. **Route Resolution**: Both static hash-map based and dynamic trie-based routing
2. **Parameter Extraction**: Automatic extraction of URL parameters into key-value maps
3. **Method Support**: All standard HTTP methods
4. **Error Handling**: Proper nullptr returns for non-existent routes
5. **Performance**: Efficient trie-based parameter matching
6. **RESTful Patterns**: Complete CRUD operation support
7. **Response Building**: Flexible response construction with status codes and headers

## Continuous Testing

The test suite is designed to be run frequently during development:

```bash
# Watch mode (requires entr or similar tool)
find ../tests -name "*.cpp" | entr -c make test

# Quick test run
make && ctest --output-on-failure
```

## Adding New Tests

To add new test cases:

1. For router tests, edit `tests/router_test.cpp`
2. For web server tests, edit `tests/web_server_test.cpp`
3. Follow the existing test structure using Catch2's `TEST_CASE` and `SECTION` macros
4. Rebuild and run tests to verify

Example:
```cpp
TEST_CASE("My new feature", "[my_tag]") {
    SECTION("Test scenario 1") {
        // Arrange
        wolf_router router;
        
        // Act
        router.get("/test", handler);
        auto [is_trie, handler, params] = router.resolve(GET, "/test");
        
        // Assert
        REQUIRE(handler != nullptr);
    }
}
```

## CI/CD Integration

Both test executables return appropriate exit codes:
- `0` - All tests passed
- Non-zero - Tests failed

This makes them suitable for integration with CI/CD pipelines:

```yaml
# Example GitHub Actions
- name: Run Tests
  run: |
    cd wolf/build
    ctest --output-on-failure
```

## Test Results Summary

**Total Test Coverage:**
- **152 assertions** across **11 test cases**
- **2 test executables**
- **100% pass rate**

---

For more information about the Wolf web server framework, see the main project documentation.
