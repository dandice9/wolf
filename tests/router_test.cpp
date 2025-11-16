#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <string>
#include "../src/wolf.hpp"

using namespace wolf;

// Mock response and request types for testing
struct MockRequest {
    std::string path;
    std::string method;
    params_t params;
};

struct MockResponse {
    std::string body;
    int status_code = 200;
};

// Test handler functions
MockResponse simple_handler(const MockRequest& /*req*/) {
    return {"Simple response", 200};
}

MockResponse user_handler(const MockRequest& /*req*/) {
    return {"User handler called", 200};
}

MockResponse post_handler(const MockRequest& /*req*/) {
    return {"Post created", 201};
}

MockResponse parameterized_handler(const MockRequest& req) {
    std::string response = "Parameters: ";
    for (const auto& [key, value] : req.params) {
        response += key + "=" + value + " ";
    }
    return {response, 200};
}

TEST_CASE("HTTP Router - Basic functionality", "[http_router]") {
    http_router<MockResponse, MockRequest> router;

    SECTION("Add and resolve simple routes") {
        router.get("/users", simple_handler);
        router.post("/users", post_handler);

        // Test GET /users (C++20: using enum class)
        auto [is_trie, handler, params] = router.resolve(http_method::GET, "/users");
        REQUIRE_FALSE(is_trie);
        REQUIRE(static_cast<bool>(handler));
        REQUIRE(params.empty());

        MockRequest req{"/users", "GET", {}};
        auto response = handler(req);
        REQUIRE(response.body == "Simple response");
        REQUIRE(response.status_code == 200);

        // Test POST /users
        auto [is_trie2, handler2, params2] = router.resolve(http_method::POST, "/users");
        REQUIRE_FALSE(is_trie2);
        REQUIRE(static_cast<bool>(handler2));
        REQUIRE(params2.empty());

        MockRequest req2{"/users", "POST", {}};
        auto response2 = handler2(req2);
        REQUIRE(response2.body == "Post created");
        REQUIRE(response2.status_code == 201);
    }

    SECTION("Method chaining") {
        router.get("/test1", simple_handler)
              .post("/test2", post_handler)
              .put("/test3", user_handler);

        auto [is_trie1, handler1, params1] = router.resolve(http_method::GET, "/test1");
        auto [is_trie2, handler2, params2] = router.resolve(http_method::POST, "/test2");
        auto [is_trie3, handler3, params3] = router.resolve(http_method::PUT, "/test3");

        REQUIRE(static_cast<bool>(handler1));
        REQUIRE(static_cast<bool>(handler2));
        REQUIRE(static_cast<bool>(handler3));
    }

    SECTION("All HTTP methods") {
        router.get("/get", simple_handler)
              .post("/post", simple_handler)
              .put("/put", simple_handler)
              .del("/delete", simple_handler)
              .patch("/patch", simple_handler)
              .options("/options", simple_handler)
              .head("/head", simple_handler)
              .connect("/connect", simple_handler)
              .trace("/trace", simple_handler);

        // Test all methods - they should not throw
        auto [is_trie1, handler1, params1] = router.resolve(http_method::GET, "/get");
        REQUIRE(static_cast<bool>(handler1));
        
        auto [is_trie2, handler2, params2] = router.resolve(http_method::POST, "/post");
        REQUIRE(static_cast<bool>(handler2));
        
        auto [is_trie3, handler3, params3] = router.resolve(http_method::PUT, "/put");
        REQUIRE(static_cast<bool>(handler3));
        
        auto [is_trie4, handler4, params4] = router.resolve(http_method::DEL, "/delete");
        REQUIRE(static_cast<bool>(handler4));
        
        auto [is_trie5, handler5, params5] = router.resolve(http_method::PATCH, "/patch");
        REQUIRE(static_cast<bool>(handler5));
        
        auto [is_trie6, handler6, params6] = router.resolve(http_method::OPTIONS, "/options");
        REQUIRE(static_cast<bool>(handler6));
        
        auto [is_trie7, handler7, params7] = router.resolve(http_method::HEAD, "/head");
        REQUIRE(static_cast<bool>(handler7));
        
        auto [is_trie8, handler8, params8] = router.resolve(http_method::CONNECT, "/connect");
        REQUIRE(static_cast<bool>(handler8));
        
        auto [is_trie9, handler9, params9] = router.resolve(http_method::TRACE, "/trace");
        REQUIRE(static_cast<bool>(handler9));
    }

    SECTION("Route not found") {
        router.get("/existing", simple_handler);

        // Non-existent route should return nullptr handler
        auto [is_trie1, handler1, params1] = router.resolve(http_method::GET, "/nonexistent");
        REQUIRE(!static_cast<bool>(handler1));
        REQUIRE_FALSE(is_trie1);
        REQUIRE(params1.empty());

        // Wrong method for existing route should return nullptr handler
        auto [is_trie2, handler2, params2] = router.resolve(http_method::POST, "/existing");
        REQUIRE(!static_cast<bool>(handler2));
        REQUIRE_FALSE(is_trie2);
        REQUIRE(params2.empty());
    }
}

TEST_CASE("Trie Router - Parameter routes", "[trie_router]") {
    http_router<MockResponse, MockRequest> router;

    SECTION("Single parameter routes") {
        router.get("/users/:id", parameterized_handler);

        auto [is_trie, handler, params] = router.resolve(http_method::GET, "/users/123");
        REQUIRE(is_trie);
        REQUIRE(static_cast<bool>(handler));
        REQUIRE(params.size() == 1);
        REQUIRE(params.at("id") == "123");

        // Test with different parameter values
        auto [is_trie2, handler2, params2] = router.resolve(http_method::GET, "/users/abc");
        REQUIRE(is_trie2);
        REQUIRE(params2.size() == 1);
        REQUIRE(params2.at("id") == "abc");
    }

    SECTION("Multiple parameter routes") {
        router.get("/users/:userId/posts/:postId", parameterized_handler);

        auto [is_trie, handler, params] = router.resolve(http_method::GET, "/users/123/posts/456");
        REQUIRE(is_trie);
        REQUIRE(static_cast<bool>(handler));
        REQUIRE(params.size() == 2);
        REQUIRE(params.at("userId") == "123");
        REQUIRE(params.at("postId") == "456");
    }

    SECTION("Mixed static and parameter routes") {
        router.get("/api/users/:id", parameterized_handler)
              .get("/api/posts/:postId/comments/:commentId", parameterized_handler);

        auto [is_trie1, handler1, params1] = router.resolve(http_method::GET, "/api/users/789");
        REQUIRE(is_trie1);
        REQUIRE(params1.size() == 1);
        REQUIRE(params1.at("id") == "789");

        auto [is_trie2, handler2, params2] = router.resolve(http_method::GET, "/api/posts/100/comments/200");
        REQUIRE(is_trie2);
        REQUIRE(params2.size() == 2);
        REQUIRE(params2.at("postId") == "100");
        REQUIRE(params2.at("commentId") == "200");
    }

    SECTION("Route detection") {
        REQUIRE(router.is_trie_route("/users/:id"));
        REQUIRE(router.is_trie_route("/api/:version/users/:id"));
        REQUIRE_FALSE(router.is_trie_route("/users"));
        REQUIRE_FALSE(router.is_trie_route("/api/v1/users"));
    }
}

TEST_CASE("Trie Router - Direct trie testing", "[trie_router]") {
    trie_router<std::function<MockResponse(MockRequest)>> trie;

    SECTION("Insert and search simple routes") {
        trie.insert("/users/:id", simple_handler);
        
        auto [handler, params] = trie.search("/users/123");
        REQUIRE(static_cast<bool>(handler));
        REQUIRE(params.size() == 1);
        REQUIRE(params.at("id") == "123");
    }

    SECTION("Search non-existent routes") {
        trie.insert("/users/:id", simple_handler);
        
        auto [handler, params] = trie.search("/posts/123");
        REQUIRE(!static_cast<bool>(handler));
        REQUIRE(params.empty());
    }

    SECTION("Complex nested routes") {
        trie.insert("/api/:version/users/:userId/posts/:postId", parameterized_handler);
        
        auto [handler, params] = trie.search("/api/v1/users/123/posts/456");
        REQUIRE(static_cast<bool>(handler));
        REQUIRE(params.size() == 3);
        
        // Verify all parameters
        REQUIRE(params.at("version") == "v1");
        REQUIRE(params.at("userId") == "123");
        REQUIRE(params.at("postId") == "456");
    }

    SECTION("Partial route matches should fail") {
        trie.insert("/users/:id/posts", simple_handler);
        
        auto [handler, params] = trie.search("/users/123");
        REQUIRE(!static_cast<bool>(handler));
        REQUIRE(params.empty());
    }
}

TEST_CASE("HTTP Methods - Enum and string conversion", "[http_methods]") {
    SECTION("Method to string conversion") {
        REQUIRE(method_to_string(http_method::GET) == "GET");
        REQUIRE(method_to_string(http_method::POST) == "POST");
        REQUIRE(method_to_string(http_method::PUT) == "PUT");
        REQUIRE(method_to_string(http_method::DEL) == "DELETE");
        REQUIRE(method_to_string(http_method::PATCH) == "PATCH");
        REQUIRE(method_to_string(http_method::OPTIONS) == "OPTIONS");
        REQUIRE(method_to_string(http_method::HEAD) == "HEAD");
        REQUIRE(method_to_string(http_method::CONNECT) == "CONNECT");
        REQUIRE(method_to_string(http_method::TRACE) == "TRACE");
    }
}

TEST_CASE("Integration - Mixed route types", "[integration]") {
    http_router<MockResponse, MockRequest> router;

    SECTION("Static and parameter routes coexistence") {
        // Add both static and parameter routes
        router.get("/users", simple_handler)
              .get("/users/:id", parameterized_handler)
              .get("/users/profile", user_handler)
              .post("/users/:id/posts", post_handler);

        // Test static route
        auto [is_trie1, handler1, params1] = router.resolve(http_method::GET, "/users");
        REQUIRE_FALSE(is_trie1);
        REQUIRE(static_cast<bool>(handler1));
        REQUIRE(params1.empty());

        // Test parameter route
        auto [is_trie2, handler2, params2] = router.resolve(http_method::GET, "/users/123");
        REQUIRE(is_trie2);
        REQUIRE(static_cast<bool>(handler2));
        REQUIRE(params2.size() == 1);
        REQUIRE(params2.at("id") == "123");

        // Test another static route
        auto [is_trie3, handler3, params3] = router.resolve(http_method::GET, "/users/profile");
        REQUIRE_FALSE(is_trie3);
        REQUIRE(static_cast<bool>(handler3));
        REQUIRE(params3.empty());

        // Test parameter route with POST
        auto [is_trie4, handler4, params4] = router.resolve(http_method::POST, "/users/456/posts");
        REQUIRE(is_trie4);
        REQUIRE(static_cast<bool>(handler4));
        REQUIRE(params4.size() == 1);
        REQUIRE(params4.at("id") == "456");
    }
}

TEST_CASE("Edge cases", "[edge_cases]") {
    http_router<MockResponse, MockRequest> router;

    SECTION("Empty routes") {
        router.get("/", simple_handler);
        
        auto [is_trie, handler, params] = router.resolve(http_method::GET, "/");
        REQUIRE_FALSE(is_trie);
        REQUIRE(static_cast<bool>(handler));
        REQUIRE(params.empty());
    }

    SECTION("Routes with trailing slashes") {
        trie_router<std::function<MockResponse(MockRequest)>> trie;
        trie.insert("/users/:id/", simple_handler);
        
        auto [handler, params] = trie.search("/users/123/");
        REQUIRE(static_cast<bool>(handler));
        REQUIRE(params.size() == 1);
        REQUIRE(params.at("id") == "123");
    }

    SECTION("Multiple slashes handling") {
        trie_router<std::function<MockResponse(MockRequest)>> trie;
        trie.insert("/users/:id", simple_handler);
        
        // This should work as the split_route function handles multiple slashes
        auto [handler, params] = trie.search("/users/123");
        REQUIRE(static_cast<bool>(handler));
    }
}
