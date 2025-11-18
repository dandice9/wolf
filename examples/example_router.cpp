#include <iostream>
#include "../src/wolf.hpp"

using namespace wolf;

// Example request/response types
struct Request {
    std::string path;
    std::string method;
    params_t params;
};

struct Response {
    std::string body;
    int status_code = 200;
};

// Example handlers
Response hello_handler(const Request& req) {
    return {"Hello, World!", 200};
}

Response user_handler(const Request& req) {
    std::string response = "User details for: ";
    if (req.params.find("id") != req.params.end()) {
        response += "ID=" + req.params.at("id");
    }
    return {response, 200};
}

Response user_posts_handler(const Request& req) {
    std::string response = "Posts for user: ";
    if (req.params.find("userId") != req.params.end()) {
        response += "UserID=" + req.params.at("userId") + " ";
    }
    if (req.params.find("postId") != req.params.end()) {
        response += "PostID=" + req.params.at("postId");
    }
    return {response, 200};
}

int main() {
    // Create router
    http_router<Response, Request> router;

    // Add routes
    router
        .get("/", hello_handler)
        .get("/users/:id", user_handler)
        .get("/users/:userId/posts/:postId", user_posts_handler)
        .post("/users", [](const Request& /*req*/) {
            return Response{"User created", 201};
        });

    // Test routes
    std::vector<std::pair<http_method, std::string>> test_routes = {
        {wolf::http_method::GET, "/"},
        {wolf::http_method::GET, "/users/123"},
        {wolf::http_method::GET, "/users/456/posts/789"},
        {wolf::http_method::POST, "/users"},
        {wolf::http_method::GET, "/not-found"}  // Test non-existent route
    };

    for (const auto& [method, path] : test_routes) {
        auto [is_trie, handler, params] = router.resolve(method, path);
        
        std::cout << method_to_string(method) << " " << path << std::endl;
        
        if (handler == nullptr) {
            std::cout << "  Route not found!" << std::endl;
            std::cout << std::endl;
            continue;
        }
        
        std::cout << "  Route type: " << (is_trie ? "Parameterized" : "Static") << std::endl;
        
        if (!params.empty()) {
            std::cout << "  Parameters: ";
            for (const auto& [key, value] : params) {
                std::cout << key << "=" << value << " ";
            }
            std::cout << std::endl;
        }

        // Create mock request
        Request req{path, std::string(method_to_string(method)), params};
        Response response = handler(req);
        
        std::cout << "  Response: " << response.body << " (Status: " << response.status_code << ")" << std::endl;
        std::cout << std::endl;
    }

    // Test route detection
    std::cout << "Route detection examples:" << std::endl;
    std::cout << "'/users/:id' is trie route: " << std::boolalpha << router.is_trie_route("/users/:id") << std::endl;
    std::cout << "'/users' is trie route: " << std::boolalpha << router.is_trie_route("/users") << std::endl;

    return 0;
}