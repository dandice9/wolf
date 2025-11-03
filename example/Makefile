# Makefile for Wolf Framework Examples
# Simple wrapper around CMake for easier building

# Detect compiler
CXX ?= g++

# Build directory
BUILD_DIR = build

# CMake options
CMAKE_OPTIONS = -DCMAKE_CXX_COMPILER=$(CXX)

.PHONY: all clean router web client help run-router run-web run-client test-server

help:
	@echo "Wolf Framework Examples - Build Commands"
	@echo "========================================"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build all examples"
	@echo "  router       - Build router example only"
	@echo "  web          - Build web server example only"
	@echo "  client       - Build HTTP test client only"
	@echo "  run-router   - Build and run router example"
	@echo "  run-web      - Build and run web server example (port 8080)"
	@echo "  run-client   - Build and run HTTP test client"
	@echo "  test-server  - Run web server and client tests"
	@echo "  clean        - Remove build directory"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make router"
	@echo "  make run-router"
	@echo "  make test-server"
	@echo ""

all: $(BUILD_DIR)
	cd $(BUILD_DIR) && $(MAKE) example_router example_web test_client

router: $(BUILD_DIR)
	cd $(BUILD_DIR) && $(MAKE) example_router

web: $(BUILD_DIR)
	cd $(BUILD_DIR) && $(MAKE) example_web

client: $(BUILD_DIR)
	cd $(BUILD_DIR) && $(MAKE) test_client

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_OPTIONS) ..

run-router: router
	@echo ""
	@echo "Running Router Example..."
	@echo "========================"
	@./$(BUILD_DIR)/example_router

run-web: web
	@echo ""
	@echo "Starting Web Server on port 8080..."
	@echo "==================================="
	@echo "Press Ctrl+C to stop"
	@echo ""
	@./$(BUILD_DIR)/example_web

run-client: client
	@echo ""
	@echo "Running HTTP Test Client..."
	@echo "============================"
	@echo "Make sure the web server is running first!"
	@echo ""
	@sleep 1
	@./$(BUILD_DIR)/test_client

test-server: web client
	@echo ""
	@echo "Starting Web Server & Running Tests..."
	@echo "======================================"
	@echo ""
	@echo "Starting server in background..."
	@./$(BUILD_DIR)/example_web > /dev/null 2>&1 & SERVER_PID=$$!; \
	sleep 2; \
	echo "Running test client..."; \
	echo ""; \
	./$(BUILD_DIR)/test_client; \
	echo ""; \
	echo "Stopping server..."; \
	kill $$SERVER_PID 2>/dev/null || true; \
	echo "Tests complete!"

clean:
	rm -rf $(BUILD_DIR)
	@echo "Build directory cleaned"

rebuild: clean all
