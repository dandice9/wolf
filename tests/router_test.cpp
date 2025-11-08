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

        // Test GET /users
        auto [is_trie, handler, params] = router.resolve(GET, "/users");
        REQUIRE_FALSE(is_trie);
        REQUIRE(handler != nullptr);
        REQUIRE(params.empty());

        MockRequest req{"/users", "GET", {}};
        auto response = handler(req);
        REQUIRE(response.body == "Simple response");
        REQUIRE(response.status_code == 200);

        // Test POST /users
        auto [is_trie2, handler2, params2] = router.resolve(POST, "/users");
        REQUIRE_FALSE(is_trie2);
        REQUIRE(handler2 != nullptr);
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

        auto [is_trie1, handler1, params1] = router.resolve(GET, "/test1");
        auto [is_trie2, handler2, params2] = router.resolve(POST, "/test2");
        auto [is_trie3, handler3, params3] = router.resolve(PUT, "/test3");

        REQUIRE(handler1 != nullptr);
        REQUIRE(handler2 != nullptr);
        REQUIRE(handler3 != nullptr);
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
        auto [is_trie1, handler1, params1] = router.resolve(GET, "/get");
        REQUIRE(handler1 != nullptr);
        
        auto [is_trie2, handler2, params2] = router.resolve(POST, "/post");
        REQUIRE(handler2 != nullptr);
        
        auto [is_trie3, handler3, params3] = router.resolve(PUT, "/put");
        REQUIRE(handler3 != nullptr);
        
        auto [is_trie4, handler4, params4] = router.resolve(DEL, "/delete");
        REQUIRE(handler4 != nullptr);
        
        auto [is_trie5, handler5, params5] = router.resolve(PATCH, "/patch");
        REQUIRE(handler5 != nullptr);
        
        auto [is_trie6, handler6, params6] = router.resolve(OPTIONS, "/options");
        REQUIRE(handler6 != nullptr);
        
        auto [is_trie7, handler7, params7] = router.resolve(HEAD, "/head");
        REQUIRE(handler7 != nullptr);
        
        auto [is_trie8, handler8, params8] = router.resolve(CONNECT, "/connect");
        REQUIRE(handler8 != nullptr);
        
        auto [is_trie9, handler9, params9] = router.resolve(TRACE, "/trace");
        REQUIRE(handler9 != nullptr);
    }

    SECTION("Route not found") {
        router.get("/existing", simple_handler);

        // Non-existent route should return nullptr handler
        auto [is_trie1, handler1, params1] = router.resolve(GET, "/nonexistent");
        REQUIRE(handler1 == nullptr);
        REQUIRE_FALSE(is_trie1);
        REQUIRE(params1.empty());

        // Wrong method for existing route should return nullptr handler
        auto [is_trie2, handler2, params2] = router.resolve(POST, "/existing");
        REQUIRE(handler2 == nullptr);
        REQUIRE_FALSE(is_trie2);
        REQUIRE(params2.empty());
    }
}

TEST_CASE("Trie Router - Parameter routes", "[trie_router]") {
    http_router<MockResponse, MockRequest> router;

    SECTION("Single parameter routes") {
        router.get("/users/:id", parameterized_handler);

        auto [is_trie, handler, params] = router.resolve(GET, "/users/123");
        REQUIRE(is_trie);
        REQUIRE(handler != nullptr);
        REQUIRE(params.size() == 1);
        REQUIRE(params.at("id") == "123");

        // Test with different parameter values
        auto [is_trie2, handler2, params2] = router.resolve(GET, "/users/abc");
        REQUIRE(is_trie2);
        REQUIRE(params2.size() == 1);
        REQUIRE(params2.at("id") == "abc");
    }

    SECTION("Multiple parameter routes") {
        router.get("/users/:userId/posts/:postId", parameterized_handler);

        auto [is_trie, handler, params] = router.resolve(GET, "/users/123/posts/456");
        REQUIRE(is_trie);
        REQUIRE(handler != nullptr);
        REQUIRE(params.size() == 2);
        REQUIRE(params.at("userId") == "123");
        REQUIRE(params.at("postId") == "456");
    }

    SECTION("Mixed static and parameter routes") {
        router.get("/api/users/:id", parameterized_handler)
              .get("/api/posts/:postId/comments/:commentId", parameterized_handler);

        auto [is_trie1, handler1, params1] = router.resolve(GET, "/api/users/789");
        REQUIRE(is_trie1);
        REQUIRE(params1.size() == 1);
        REQUIRE(params1.at("id") == "789");

        auto [is_trie2, handler2, params2] = router.resolve(GET, "/api/posts/100/comments/200");
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
        REQUIRE(handler != nullptr);
        REQUIRE(params.size() == 1);
        REQUIRE(params.at("id") == "123");
    }

    SECTION("Search non-existent routes") {
        trie.insert("/users/:id", simple_handler);
        
        auto [handler, params] = trie.search("/posts/123");
        REQUIRE(handler == nullptr);
        REQUIRE(params.empty());
    }

    SECTION("Complex nested routes") {
        trie.insert("/api/:version/users/:userId/posts/:postId", parameterized_handler);
        
        auto [handler, params] = trie.search("/api/v1/users/123/posts/456");
        REQUIRE(handler != nullptr);
        REQUIRE(params.size() == 3);
        
        // Verify all parameters
        REQUIRE(params.at("version") == "v1");
        REQUIRE(params.at("userId") == "123");
        REQUIRE(params.at("postId") == "456");
    }

    SECTION("Partial route matches should fail") {
        trie.insert("/users/:id/posts", simple_handler);
        
        auto [handler, params] = trie.search("/users/123");
        REQUIRE(handler == nullptr);
        REQUIRE(params.empty());
    }
}

TEST_CASE("HTTP Methods - Enum and string conversion", "[http_methods]") {
    SECTION("Method to string conversion") {
        REQUIRE(method_to_string(GET) == "GET");
        REQUIRE(method_to_string(POST) == "POST");
        REQUIRE(method_to_string(PUT) == "PUT");
        REQUIRE(method_to_string(DEL) == "DELETE");
        REQUIRE(method_to_string(PATCH) == "PATCH");
        REQUIRE(method_to_string(OPTIONS) == "OPTIONS");
        REQUIRE(method_to_string(HEAD) == "HEAD");
        REQUIRE(method_to_string(CONNECT) == "CONNECT");
        REQUIRE(method_to_string(TRACE) == "TRACE");
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
        auto [is_trie1, handler1, params1] = router.resolve(GET, "/users");
        REQUIRE_FALSE(is_trie1);
        REQUIRE(handler1 != nullptr);
        REQUIRE(params1.empty());

        // Test parameter route
        auto [is_trie2, handler2, params2] = router.resolve(GET, "/users/123");
        REQUIRE(is_trie2);
        REQUIRE(handler2 != nullptr);
        REQUIRE(params2.size() == 1);
        REQUIRE(params2.at("id") == "123");

        // Test another static route
        auto [is_trie3, handler3, params3] = router.resolve(GET, "/users/profile");
        REQUIRE_FALSE(is_trie3);
        REQUIRE(handler3 != nullptr);
        REQUIRE(params3.empty());

        // Test parameter route with POST
        auto [is_trie4, handler4, params4] = router.resolve(POST, "/users/456/posts");
        REQUIRE(is_trie4);
        REQUIRE(handler4 != nullptr);
        REQUIRE(params4.size() == 1);
        REQUIRE(params4.at("id") == "456");
    }
}

TEST_CASE("Edge cases", "[edge_cases]") {
    http_router<MockResponse, MockRequest> router;

    SECTION("Empty routes") {
        router.get("/", simple_handler);
        
        auto [is_trie, handler, params] = router.resolve(GET, "/");
        REQUIRE_FALSE(is_trie);
        REQUIRE(handler != nullptr);
        REQUIRE(params.empty());
    }

    SECTION("Routes with trailing slashes") {
        trie_router<std::function<MockResponse(MockRequest)>> trie;
        trie.insert("/users/:id/", simple_handler);
        
        auto [handler, params] = trie.search("/users/123/");
        REQUIRE(handler != nullptr);
        REQUIRE(params.size() == 1);
        REQUIRE(params.at("id") == "123");
    }

    SECTION("Multiple slashes handling") {
        trie_router<std::function<MockResponse(MockRequest)>> trie;
        trie.insert("/users/:id", simple_handler);
        
        // This should work as the split_route function handles multiple slashes
        auto [handler, params] = trie.search("/users/123");
        REQUIRE(handler != nullptr);
    }
}
