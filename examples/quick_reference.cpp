#include "../src/wolf.hpp"
#include <iostream>
#include <boost/json.hpp>

namespace json = boost::json;

int main() {
    try {
        wolf::web_server server(8080);

        // ============================================================
        // QUICK REFERENCE: Common HTTP Response Patterns
        // ============================================================

        // 1. GET - Success with JSON data (200 OK)
        server->get("/api/status", [](const wolf::http_request& req) {
            return wolf::http_response(200).json(json::object{
                {"status", "ok"},
                {"timestamp", std::time(nullptr)}
            });
        });

        // 2. POST - Resource created (201 Created)
        server->post("/api/users", [](const wolf::http_request& req) {
            auto body = req.get_json_body();
            return wolf::http_response(201).json(json::object{
                {"success", true},
                {"id", 123},
                {"message", "User created"}
            });
        });

        // 3. PUT - Resource updated (200 OK)
        server->put("/api/users/:id", [](const wolf::http_request& req) {
            auto id = req.get_or("id", "0");
            return wolf::http_response(200).json(json::object{
                {"success", true},
                {"id", id},
                {"message", "User updated"}
            });
        });

        // 4. DELETE - Resource deleted (204 No Content)
        server->del("/api/users/:id", [](const wolf::http_request& req) {
            // 204 responses typically have no body
            return wolf::http_response(204).text("");
        });

        // 5. GET - Not found (404 Not Found)
        server->get("/api/users/:id", [](const wolf::http_request& req) {
            auto id = req.get_or("id", "0");
            // Simulate not found
            bool exists = false;
            
            if (!exists) {
                return wolf::http_response(404).json(json::object{
                    {"error", "Not Found"},
                    {"message", "User not found"},
                    {"user_id", id}
                });
            }
            
            return wolf::http_response(200).json(json::object{{"id", id}});
        });

        // 6. POST - Validation error (422 Unprocessable Entity)
        server->post("/api/validate", [](const wolf::http_request& req) {
            return wolf::http_response(422).json(json::object{
                {"success", false},
                {"errors", json::array{
                    json::object{{"field", "email"}, {"message", "Invalid format"}},
                    json::object{{"field", "password"}, {"message", "Too short"}}
                }}
            });
        });

        // 7. GET - Unauthorized (401 Unauthorized)
        server->get("/api/protected", [](const wolf::http_request& req) {
            auto auth = req[boost::beast::http::field::authorization];
            
            if (auth.empty()) {
                return wolf::http_response(401)
                    .header("WWW-Authenticate", "Bearer")
                    .json(json::object{
                        {"error", "Unauthorized"},
                        {"message", "Authentication required"}
                    });
            }
            
            return wolf::http_response(200).json(json::object{{"data", "secret"}});
        });

        // 8. GET - Bad request (400 Bad Request)
        server->get("/api/calculate", [](const wolf::http_request& req) {
            auto params = req.query_params();
            
            if (params.find("value") == params.end()) {
                return wolf::http_response(400).json(json::object{
                    {"error", "Bad Request"},
                    {"message", "Missing required parameter: value"}
                });
            }
            
            return wolf::http_response(200).json(json::object{{"result", 42}});
        });

        // 9. GET - Server error (500 Internal Server Error)
        server->get("/api/error", [](const wolf::http_request& req) {
            return wolf::http_response(500).json(json::object{
                {"error", "Internal Server Error"},
                {"message", "Something went wrong"},
                {"timestamp", std::time(nullptr)}
            });
        });

        // 10. GET - With custom headers
        server->get("/api/info", [](const wolf::http_request& req) {
            return wolf::http_response(200)
                .header("X-API-Version", "1.0")
                .header("X-RateLimit-Limit", "1000")
                .header("X-RateLimit-Remaining", "999")
                .json(json::object{{"info", "API information"}});
        });

        // 11. POST - Login with cookie
        server->post("/api/login", [](const wolf::http_request& req) {
            auto body = req.get_json_body();
            
            return wolf::http_response(200)
                .json(json::object{
                    {"success", true},
                    {"message", "Login successful"}
                })
                .cookie("session_id", "abc123def456", "/", "", 3600, true, false);
        });

        // 12. GET - Logout (clear cookie)
        server->get("/api/logout", [](const wolf::http_request& req) {
            return wolf::http_response(200)
                .json(json::object{{"message", "Logged out"}})
                .cookie("session_id", "", "/", "", 0, true, false);
        });

        // 13. GET - Array response
        server->get("/api/items", [](const wolf::http_request& req) {
            json::array items = {
                json::object{{"id", 1}, {"name", "Item 1"}},
                json::object{{"id", 2}, {"name", "Item 2"}},
                json::object{{"id", 3}, {"name", "Item 3"}}
            };
            return wolf::http_response(200).json(items);
        });

        // 14. GET - Plain text
        server->get("/health", [](const wolf::http_request& req) {
            return wolf::http_response(200).text("OK");
        });

        // 15. GET - HTML page
        server->get("/", [](const wolf::http_request& req) {
            return wolf::http_response(200).html(R"(
<!DOCTYPE html>
<html>
<head>
    <title>Wolf API - Quick Reference</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 50px auto; }
        h1 { color: #333; }
        .endpoint { background: #f5f5f5; padding: 10px; margin: 10px 0; border-radius: 5px; }
        .method { font-weight: bold; color: #0066cc; }
    </style>
</head>
<body>
    <h1>🐺 Wolf API - Quick Reference</h1>
    <div class="endpoint"><span class="method">GET</span> /api/status - Server status</div>
    <div class="endpoint"><span class="method">POST</span> /api/users - Create user</div>
    <div class="endpoint"><span class="method">PUT</span> /api/users/:id - Update user</div>
    <div class="endpoint"><span class="method">DELETE</span> /api/users/:id - Delete user</div>
    <div class="endpoint"><span class="method">GET</span> /api/items - List items</div>
    <div class="endpoint"><span class="method">POST</span> /api/login - Login (sets cookie)</div>
    <div class="endpoint"><span class="method">GET</span> /api/logout - Logout (clears cookie)</div>
    <div class="endpoint"><span class="method">GET</span> /health - Health check</div>
</body>
</html>
            )");
        });

        std::cout << "🐺 Wolf Quick Reference Server\n";
        std::cout << "================================\n\n";
        std::cout << "Server running on http://localhost:8080\n\n";
        
        std::cout << "Common HTTP Response Patterns:\n";
        std::cout << "  GET    /                     - HTML home page\n";
        std::cout << "  GET    /health               - Plain text health check\n";
        std::cout << "  GET    /api/status           - 200 OK JSON response\n";
        std::cout << "  POST   /api/users            - 201 Created\n";
        std::cout << "  PUT    /api/users/:id        - 200 OK Updated\n";
        std::cout << "  DELETE /api/users/:id        - 204 No Content\n";
        std::cout << "  GET    /api/users/:id        - 404 Not Found example\n";
        std::cout << "  POST   /api/validate         - 422 Validation Error\n";
        std::cout << "  GET    /api/protected        - 401 Unauthorized\n";
        std::cout << "  GET    /api/calculate        - 400 Bad Request\n";
        std::cout << "  GET    /api/error            - 500 Server Error\n";
        std::cout << "  GET    /api/info             - Response with headers\n";
        std::cout << "  POST   /api/login            - Set cookie\n";
        std::cout << "  GET    /api/logout           - Clear cookie\n";
        std::cout << "  GET    /api/items            - JSON array\n\n";
        
        std::cout << "Try these commands:\n";
        std::cout << "  curl http://localhost:8080/health\n";
        std::cout << "  curl http://localhost:8080/api/status\n";
        std::cout << "  curl -X POST http://localhost:8080/api/users\n";
        std::cout << "  curl http://localhost:8080/api/items\n";
        std::cout << "  curl -X POST http://localhost:8080/api/login\n\n";
        
        std::cout << "Press Ctrl+C to stop\n\n";

        server.start();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
