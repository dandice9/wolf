# Quick Build Reference

## Building Examples

### From main project directory:
```bash
cd wolf/build
cmake ..
make example_router example_web

# Run examples
./example/example_router
./example/example_web
```

### From example directory (standalone):
```bash
cd wolf/example
make all           # Build all examples
make router        # Build router example only
make web          # Build web server example only
make run-router   # Build and run router example
make run-web      # Build and run web server example
make clean        # Clean build files
```

## Building Tests

```bash
cd wolf/build
cmake ..
make router_tests web_server_tests

# Run all tests
ctest --verbose

# Or run individually
./router_tests
./web_server_tests
```

## Complete Build

```bash
cd wolf
mkdir -p build
cd build

# Configure
cmake ..

# Build tests
make router_tests web_server_tests

# Build examples
make example_router example_web

# Run tests
ctest --verbose

# Run examples
./example/example_router
```

## Directory Structure

```
wolf/
├── CMakeLists.txt           # Main CMake configuration
├── http_router.hpp          # Router implementation
├── web_server.hpp           # Web server implementation
├── example/
│   ├── CMakeLists.txt       # Example builds configuration
│   ├── Makefile            # Convenience Makefile
│   ├── README.md           # Example documentation
│   ├── example_router.cpp  # Router example
│   └── example_web.cpp     # Web server example
├── tests/
│   ├── README.md           # Test documentation
│   ├── router_test.cpp     # Router tests
│   └── web_server_test.cpp # Web server tests
└── build/
    ├── router_tests        # Test executable
    ├── web_server_tests    # Test executable
    └── example/
        ├── example_router  # Example executable
        └── example_web     # Example executable
```

## CMake Targets

| Target | Description |
|--------|-------------|
| `router_tests` | Router test suite |
| `web_server_tests` | Web server test suite |
| `example_router` | Router example |
| `example_web` | Web server example |

## Quick Commands

```bash
# Test everything
cd wolf/build && ctest

# Run router example
cd wolf/example && make run-router

# Run web server example
cd wolf/example && make run-web

# Clean everything
cd wolf/build && make clean
cd wolf/example && make clean
```
