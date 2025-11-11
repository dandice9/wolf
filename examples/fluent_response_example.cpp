#include "../src/wolf.hpp"
#include <iostream>
#include <boost/json.hpp>

namespace json = boost::json;

int main() {
    try {
        wolf::web_server server(8080);

        // Example 1: Simple JSON response with status code
        server->get("/api/user", [](const wolf::http_request& req) {
            json::object user = {
                {"id", 1},
                {"name", "John Doe"},
                {"email", "john@example.com"}
            };
            return wolf::http_response(200).json(user);
        });

        // Example 2: Created response (201) with JSON
        server->post("/api/user", [](const wolf::http_request& req) {
            auto body = req.get_json_body();
            
            json::object response = {
                {"success", true},
                {"message", "User created successfully"},
                {"data", body}
            };
            
            return wolf::http_response(201).json(response);
        });

        // Example 3: Error response with JSON
        server->get("/api/error", [](const wolf::http_request& req) {
            json::object error = {
                {"error", "Not Found"},
                {"message", "The requested resource was not found"},
                {"code", 404}
            };
            return wolf::http_response(404).json(error);
        });

        // Example 4: Array response
        server->get("/api/users", [](const wolf::http_request& req) {
            json::array users = {
                json::object{{"id", 1}, {"name", "Alice"}},
                json::object{{"id", 2}, {"name", "Bob"}},
                json::object{{"id", 3}, {"name", "Charlie"}}
            };
            return wolf::http_response(200).json(users);
        });

        // Example 5: Text response
        server->get("/api/text", [](const wolf::http_request& req) {
            return wolf::http_response(200).text("Hello, World!");
        });

        // Example 6: HTML response
        server->get("/", [](const wolf::http_request& req) {
            return wolf::http_response(200).html(R"(
                <!DOCTYPE html>
                <html>
                <head><title>Wolf Server</title></head>
                <body>
                    <h1>Welcome to Wolf Web Server</h1>
                    <p>Using fluent API for responses!</p>
                </body>
                </html>
            )");
        });

        // Example 7: Custom headers
        server->get("/api/custom-headers", [](const wolf::http_request& req) {
            json::object data = {{"message", "Custom headers example"}};
            
            return wolf::http_response(200)
                .json(data)
                .header("X-Custom-Header", "MyValue")
                .header("X-Request-ID", "12345");
        });

        // Example 8: Set cookie with response
        server->get("/api/login", [](const wolf::http_request& req) {
            json::object response = {
                {"success", true},
                {"message", "Login successful"}
            };
            
            return wolf::http_response(200)
                .json(response)
                .cookie("session_id", "abc123def456", "/", "", 3600, true, false);
        });

        // Example 9: Chain multiple operations
        server->get("/api/complex", [](const wolf::http_request& req) {
            json::object data = {
                {"status", "success"},
                {"timestamp", std::time(nullptr)},
                {"data", json::object{
                    {"items", json::array{1, 2, 3, 4, 5}}
                }}
            };
            
            return wolf::http_response(200)
                .header("X-API-Version", "1.0")
                .header("X-RateLimit-Remaining", "99")
                .json(data)
                .cookie("tracking_id", "xyz789", "/", "", 86400, false, false);
        });

        // Example 10: Conditional response based on query params
        server->get("/api/data", [](const wolf::http_request& req) {
            auto format = req.get_or("format", "json");
            
            if (format == "json") {
                json::object data = {{"message", "JSON format"}};
                return wolf::http_response(200).json(data);
            } else if (format == "text") {
                return wolf::http_response(200).text("Plain text format");
            } else if (format == "html") {
                return wolf::http_response(200).html("<h1>HTML format</h1>");
            } else {
                json::object error = {
                    {"error", "Invalid format"},
                    {"message", "Supported formats: json, text, html"}
                };
                return wolf::http_response(400).json(error);
            }
        });

        // Example 11: File download
        server->get("/api/download", [](const wolf::http_request& req) {
            std::string file_content = "This is a sample file content.";
            
            return wolf::http_response(200)
                .send_file("sample.txt", "text/plain")
                .text(file_content);
        });

        // Example 12: Validation error with detailed JSON
        server->post("/api/validate", [](const wolf::http_request& req) {
            auto body = req.get_json_body();
            
            // Simulate validation
            json::object validation_error = {
                {"success", false},
                {"errors", json::array{
                    json::object{{"field", "email"}, {"message", "Invalid email format"}},
                    json::object{{"field", "password"}, {"message", "Password too short"}}
                }}
            };
            
            return wolf::http_response(422).json(validation_error);
        });

        // Example 13: No Content response
        server->delete_("/api/user/:id", [](const wolf::http_request& req) {
            // Simulate deletion
            return wolf::http_response(204).text("");
        });

        // Example 14: Redirect (moved permanently)
        server->get("/old-path", [](const wolf::http_request& req) {
            return wolf::http_response(301)
                .header("Location", "/new-path")
                .text("Moved Permanently");
        });

        // Example 15: Server error with JSON
        server->get("/api/server-error", [](const wolf::http_request& req) {
            json::object error = {
                {"error", "Internal Server Error"},
                {"message", "An unexpected error occurred"},
                {"code", 500},
                {"timestamp", std::time(nullptr)}
            };
            return wolf::http_response(500).json(error);
        });

        std::cout << "Server started on http://localhost:8080\n";
        std::cout << "\nAvailable endpoints:\n";
        std::cout << "  GET  /                       - HTML home page\n";
        std::cout << "  GET  /api/user              - Get user (JSON)\n";
        std::cout << "  POST /api/user              - Create user (JSON)\n";
        std::cout << "  GET  /api/users             - Get all users (JSON array)\n";
        std::cout << "  GET  /api/text              - Plain text response\n";
        std::cout << "  GET  /api/custom-headers    - Response with custom headers\n";
        std::cout << "  GET  /api/login             - Login with cookie\n";
        std::cout << "  GET  /api/complex           - Complex chained response\n";
        std::cout << "  GET  /api/data?format=json  - Conditional response\n";
        std::cout << "  GET  /api/download          - File download\n";
        std::cout << "  POST /api/validate          - Validation error example\n";
        std::cout << "  DEL  /api/user/:id          - Delete user (204)\n";
        std::cout << "  GET  /api/error             - 404 error example\n";
        std::cout << "  GET  /api/server-error      - 500 error example\n";
        std::cout << "\nPress Ctrl+C to stop the server.\n\n";

        server.start();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
