#include "../src/web_server.hpp"
#include <iostream>
#include <csignal>

int main() {
    std::cout << "Starting Wolf Web Server on http://localhost:8080" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    wolf::web_server app(8080);

    app->set_socket_handler([](const std::string& msg) {
        // Echo the received message back to the client
        return "Echo: " + msg;
    });

    // GET: Root endpoint
    app->get("/", [](const wolf::http_request& /*req*/) {
        wolf::response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/html");
        res.body() = R"(
            <html>
            <head><title>Wolf Server</title></head>
            <body>
                <h1>Welcome to Wolf Server!</h1>
                <p>Server is running successfully.</p>
                <h2>Available Endpoints:</h2>
                <ul>
                    <li>GET / - This page</li>
                    <li>GET /api/users - List all users</li>
                    <li>GET /api/users/:id - Get user by ID</li>
                    <li>POST /api/users - Create new user</li>
                    <li>PUT /api/users/:id - Update user</li>
                    <li>DELETE /api/users/:id - Delete user</li>
                    <li>GET /health - Health check</li>
                </ul>
            </body>
            </html>
        )";
        return res;
    });

    // ping endpoint - simple string return
    app->get("/ping", [](const wolf::http_request& /*req*/) {
        return "pong";
    });
    
    app->get("/hello", [](auto req) {
        return "hello world";
    });

    // Cookie examples
    app->get("/set-cookie", [](const wolf::http_request& /*req*/) {
        wolf::response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"message": "Cookie set successfully"})";
        
        // Set a cookie with 1 hour expiration
        wolf::set_cookie(res, "session_id", "demo_session_12345", "/", "", 3600);
        
        return res;
    });

    app->get("/get-cookie", [](const wolf::http_request& req) {
        auto cookies = req.cookies();
        
        wolf::response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        
        if (cookies.find("session_id") != cookies.end()) {
            res.body() = R"({"session_id": ")" + cookies.at("session_id") + R"("})";
        } else {
            res.body() = R"({"error": "No session cookie found"})";
        }
        
        return res;
    });

    app->get("/hello", [](const wolf::http_request& req) {
        auto cookies = req.cookies();
        
        std::string greeting = "Hello, Guest!";
        if (cookies.find("username") != cookies.end()) {
            greeting = "Hello, " + cookies.at("username") + "!";
        }

        return greeting;
    });

    // GET: List all users
    app->get("/api/users", [](const wolf::http_request& /*req*/) {
        wolf::response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"([
            {"id": 1, "name": "Alice", "email": "alice@example.com"},
            {"id": 2, "name": "Bob", "email": "bob@example.com"},
            {"id": 3, "name": "Charlie", "email": "charlie@example.com"}
        ])";
        return res;
    });

    // GET: Get user by ID
    app->get("/api/users/:id", [](const wolf::http_request& req) {
        wolf::response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"id": )" + req.get("id") + R"(, "name": "Alice", "email": "alice@example.com"})";
        return res;
    });

    // POST: Create new user
    app->post("/api/users", [](const wolf::http_request& req) {
        wolf::response_t res;
        res.result(http::status::created);
        res.set(http::field::content_type, "application/json");
        
        std::string body = req.body();
        if (body.empty()) {
            body = R"({"name": "New User", "email": "newuser@example.com"})";
        }
        
        res.body() = R"({"id": 4, "created": true, "data": )" + body + "}";
        return res;
    });

    // PUT: Update user by ID
    app->put("/api/users/:id", [](const wolf::http_request& req) {
        wolf::response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        
        std::string body = req.body();
        if (body.empty()) {
            body = R"({"name": "Updated User", "email": "updated@example.com"})";
        }
        
        res.body() = R"({"id": 1, "updated": true, "data": )" + body + "}";
        return res;
    });

    // DELETE: Delete user by ID
    app->del("/api/users/:id", [](const wolf::http_request& /*req*/) {
        wolf::response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"deleted": true, "id": 1})";
        return res;
    });

    // PATCH: Partial update user by ID
    app->patch("/api/users/:id", [](const wolf::http_request& req) {
        wolf::response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        
        std::string body = req.body();
        if (body.empty()) {
            body = R"({"email": "patched@example.com"})";
        }

        auto id = req.get("id");

        res.body() = R"({"id": )" + id + R"(, "patched": true, "data": )" + body + "}";
        return res;
    });

    // GET: Health check
    app->get("/health", [](const wolf::http_request& /*req*/) {
        wolf::response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"status": "healthy", "timestamp": "2025-11-03T12:00:00Z"})";
        return res;
    });

    std::cout << "Server started successfully!" << std::endl;
    std::cout << "Try: curl http://localhost:8080" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    // Start the server (blocks until stopped)
    app.start();

    return 0;
}