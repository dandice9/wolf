#include "../src/wolf.hpp"
#include <iostream>
#include <boost/json.hpp>

namespace json = boost::json;

int main() {
    try {
        wolf::web_server server(8080);

        // ============================================================
        // NEW: Handlers can now return wolf::http_response directly!
        // This enables the fluent API for all route handlers
        // ============================================================

        // Example 1: Simple GET with http_response
        server->get("/api/hello", [](const wolf::http_request& req) -> wolf::http_response {
            return wolf::http_response(200).json(json::object{
                {"message", "Hello, World!"}
            });
        });

        // Example 2: POST with 201 Created
        server->post("/api/users", [](const wolf::http_request& req) -> wolf::http_response {
            auto body = req.get_json_body();
            
            // Simulate user creation
            json::object result = {
                {"success", true},
                {"id", 123},
                {"created_at", std::time(nullptr)}
            };
            
            return wolf::http_response(201).json(result);
        });

        // Example 3: GET with route parameters
        server->get("/api/users/:id", [](const wolf::http_request& req) -> wolf::http_response {
            auto id = req.get_or("id", "0");
            
            // Simulate not found
            if (id == "999") {
                return wolf::http_response(404).json(json::object{
                    {"error", "Not Found"},
                    {"message", "User not found"},
                    {"user_id", id}
                });
            }
            
            // Return user data
            return wolf::http_response(200).json(json::object{
                {"id", id},
                {"name", "John Doe"},
                {"email", "john@example.com"}
            });
        });

        // Example 4: PUT with http_response
        server->put("/api/users/:id", [](const wolf::http_request& req) -> wolf::http_response {
            auto id = req.get_or("id", "0");
            auto body = req.get_json_body();
            
            return wolf::http_response(200).json(json::object{
                {"success", true},
                {"id", id},
                {"message", "User updated"}
            });
        });

        // Example 5: DELETE with 204 No Content
        server->delete_("/api/users/:id", [](const wolf::http_request& req) -> wolf::http_response {
            auto id = req.get_or("id", "0");
            
            // Simulate deletion
            return wolf::http_response(204).text("");
        });

        // Example 6: Response with custom headers
        server->get("/api/info", [](const wolf::http_request& req) -> wolf::http_response {
            json::object data = {
                {"version", "1.0"},
                {"status", "operational"}
            };
            
            return wolf::http_response(200)
                .header("X-API-Version", "1.0")
                .header("X-RateLimit-Limit", "1000")
                .header("X-RateLimit-Remaining", "999")
                .json(data);
        });

        // Example 7: Login with cookie
        server->post("/api/login", [](const wolf::http_request& req) -> wolf::http_response {
            auto body = req.get_json_body();
            
            // Simulate authentication
            std::string session_id = "abc123def456";
            
            return wolf::http_response(200)
                .json(json::object{
                    {"success", true},
                    {"message", "Login successful"}
                })
                .cookie("session_id", session_id, "/", "", 3600, true, false);
        });

        // Example 8: Validation error
        server->post("/api/validate", [](const wolf::http_request& req) -> wolf::http_response {
            auto body = req.get_json_body();
            
            // Simulate validation
            json::object error = {
                {"success", false},
                {"errors", json::array{
                    json::object{{"field", "email"}, {"message", "Invalid email format"}},
                    json::object{{"field", "password"}, {"message", "Password too short"}}
                }}
            };
            
            return wolf::http_response(422).json(error);
        });

        // Example 9: Text response
        server->get("/health", [](const wolf::http_request& req) -> wolf::http_response {
            return wolf::http_response(200).text("OK");
        });

        // Example 10: HTML response
        server->get("/", [](const wolf::http_request& req) -> wolf::http_response {
            return wolf::http_response(200).html(R"(
<!DOCTYPE html>
<html>
<head>
    <title>Wolf API - http_response Demo</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 50px auto; padding: 20px; }
        h1 { color: #333; }
        .endpoint { background: #f5f5f5; padding: 15px; margin: 10px 0; border-radius: 5px; border-left: 4px solid #0066cc; }
        .method { font-weight: bold; color: #0066cc; display: inline-block; width: 80px; }
        .path { font-family: monospace; color: #666; }
        code { background: #e8e8e8; padding: 2px 6px; border-radius: 3px; }
    </style>
</head>
<body>
    <h1>🐺 Wolf API - http_response Return Type Demo</h1>
    <p>All handlers now support returning <code>wolf::http_response</code> for fluent API usage!</p>
    
    <h2>Available Endpoints:</h2>
    
    <div class="endpoint">
        <span class="method">GET</span>
        <span class="path">/api/hello</span><br>
        Simple JSON response
    </div>
    
    <div class="endpoint">
        <span class="method">POST</span>
        <span class="path">/api/users</span><br>
        Create user (returns 201)
    </div>
    
    <div class="endpoint">
        <span class="method">GET</span>
        <span class="path">/api/users/:id</span><br>
        Get user by ID (try 999 for 404)
    </div>
    
    <div class="endpoint">
        <span class="method">PUT</span>
        <span class="path">/api/users/:id</span><br>
        Update user
    </div>
    
    <div class="endpoint">
        <span class="method">DELETE</span>
        <span class="path">/api/users/:id</span><br>
        Delete user (returns 204)
    </div>
    
    <div class="endpoint">
        <span class="method">GET</span>
        <span class="path">/api/info</span><br>
        API info with custom headers
    </div>
    
    <div class="endpoint">
        <span class="method">POST</span>
        <span class="path">/api/login</span><br>
        Login (sets cookie)
    </div>
    
    <div class="endpoint">
        <span class="method">POST</span>
        <span class="path">/api/validate</span><br>
        Validation error example (returns 422)
    </div>
    
    <div class="endpoint">
        <span class="method">GET</span>
        <span class="path">/health</span><br>
        Health check (plain text)
    </div>
    
    <h2>Try These Commands:</h2>
    <pre>
curl http://localhost:8080/health
curl http://localhost:8080/api/hello
curl http://localhost:8080/api/users/123
curl http://localhost:8080/api/users/999
curl -X POST http://localhost:8080/api/users
curl -X POST http://localhost:8080/api/login
curl -X DELETE http://localhost:8080/api/users/123
curl -i http://localhost:8080/api/info
    </pre>
</body>
</html>
            )");
        });

        // Example 11: Complex response with multiple features
        server->post("/api/complex", [](const wolf::http_request& req) -> wolf::http_response {
            json::object data = {
                {"request_id", "req-12345"},
                {"timestamp", std::time(nullptr)},
                {"data", json::object{
                    {"items", json::array{1, 2, 3, 4, 5}}
                }}
            };
            
            return wolf::http_response(200)
                .header("X-Request-ID", "req-12345")
                .header("X-API-Version", "2.0")
                .header("X-RateLimit-Limit", "1000")
                .header("X-RateLimit-Remaining", "999")
                .header("X-RateLimit-Reset", "1699999999")
                .json(data)
                .cookie("tracking_id", "xyz789", "/", "", 86400, false, false);
        });

        // Example 12: Error handling with try-catch
        server->get("/api/error-demo", [](const wolf::http_request& req) -> wolf::http_response {
            try {
                // Simulate some operation that might fail
                bool simulate_error = req.query_params().count("error") > 0;
                
                if (simulate_error) {
                    throw std::runtime_error("Simulated error");
                }
                
                return wolf::http_response(200).json(json::object{
                    {"message", "Success"}
                });
            } catch (const std::exception& e) {
                return wolf::http_response(500).json(json::object{
                    {"error", "Internal Server Error"},
                    {"message", e.what()},
                    {"timestamp", std::time(nullptr)}
                });
            }
        });

        // Example 13: Conditional response based on query params
        server->get("/api/data", [](const wolf::http_request& req) -> wolf::http_response {
            auto format = req.get_or("format", "json");
            
            if (format == "json") {
                return wolf::http_response(200).json(json::object{
                    {"format", "json"},
                    {"data", json::array{1, 2, 3}}
                });
            } else if (format == "text") {
                return wolf::http_response(200).text("Plain text data: 1, 2, 3");
            } else if (format == "html") {
                return wolf::http_response(200).html("<ul><li>1</li><li>2</li><li>3</li></ul>");
            } else {
                return wolf::http_response(400).json(json::object{
                    {"error", "Bad Request"},
                    {"message", "Invalid format parameter"},
                    {"supported", json::array{"json", "text", "html"}}
                });
            }
        });

        std::cout << "🐺 Wolf Server - http_response Return Type Demo\n";
        std::cout << "================================================\n\n";
        std::cout << "✨ NEW: All handlers can now return wolf::http_response!\n\n";
        std::cout << "Server running on http://localhost:8080\n\n";
        
        std::cout << "Available endpoints:\n";
        std::cout << "  GET    /                     - HTML home page\n";
        std::cout << "  GET    /health               - Health check\n";
        std::cout << "  GET    /api/hello            - Simple JSON\n";
        std::cout << "  POST   /api/users            - Create user (201)\n";
        std::cout << "  GET    /api/users/:id        - Get user\n";
        std::cout << "  PUT    /api/users/:id        - Update user\n";
        std::cout << "  DELETE /api/users/:id        - Delete user (204)\n";
        std::cout << "  GET    /api/info             - With custom headers\n";
        std::cout << "  POST   /api/login            - Sets cookie\n";
        std::cout << "  POST   /api/validate         - Validation error (422)\n";
        std::cout << "  POST   /api/complex          - Complex response\n";
        std::cout << "  GET    /api/error-demo       - Error handling\n";
        std::cout << "  GET    /api/data?format=X    - Conditional response\n\n";
        
        std::cout << "Try:\n";
        std::cout << "  curl http://localhost:8080/api/hello\n";
        std::cout << "  curl http://localhost:8080/api/users/123\n";
        std::cout << "  curl http://localhost:8080/api/data?format=json\n";
        std::cout << "  curl -i http://localhost:8080/api/info\n\n";
        
        std::cout << "Press Ctrl+C to stop\n\n";

        server.start();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
