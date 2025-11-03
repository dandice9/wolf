#include <catch2/catch_test_macros.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include "../web_server.hpp"

using namespace wolf;
namespace http = boost::beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

TEST_CASE("HTTP Request - Parameter handling", "[http_request]") {
    SECTION("URI parameters") {
        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/users/123");
        
        params_t uri_params;
        uri_params["id"] = "123";
        
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);
        
        // Note: http_request is derived from request_t, so we can test the basic properties
        REQUIRE(req.method() == http::verb::get);
        REQUIRE(req.target() == "/users/123");
    }

    SECTION("Query parameters") {
        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/search?q=test&page=1");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("q=test&page=1");
        http_request req(base_req, uri_params, query_result);
        
        REQUIRE(req.method() == http::verb::get);
        REQUIRE(req.target() == "/search?q=test&page=1");
    }

    SECTION("Empty parameters") {
        http::request<http::string_body> base_req;
        base_req.method(http::verb::post);
        base_req.target("/api/data");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);
        
        REQUIRE(req.method() == http::verb::post);
        REQUIRE(req.target() == "/api/data");
    }
}

TEST_CASE("Wolf Router - HTTP method mapping", "[wolf_router]") {
    wolf_router router;

    SECTION("Basic route registration") {
        bool handler_called = false;
        
        router.get("/test", [&handler_called](const http_request& /*req*/) {
            handler_called = true;
            response_t res;
            res.result(http::status::ok);
            res.body() = "Test response";
            return res;
        });

        auto [is_trie, handler, params] = router.resolve(GET, "/test");
        REQUIRE_FALSE(is_trie);
        REQUIRE(handler != nullptr);
        REQUIRE(params.empty());
        
        // Call the handler
        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/test");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);
        
        auto response = handler(req);
        REQUIRE(handler_called);
        REQUIRE(response.result() == http::status::ok);
        REQUIRE(response.body() == "Test response");
    }

    SECTION("Parameterized routes") {
        router.get("/users/:id", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            res.body() = "User details";
            return res;
        });

        auto [is_trie, handler, params] = router.resolve(GET, "/users/123");
        REQUIRE(is_trie);
        REQUIRE(handler != nullptr);
        REQUIRE(params.size() == 1);
        REQUIRE(params.at("id") == "123");
    }

    SECTION("Multiple HTTP methods on same path") {
        router.get("/resource", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            res.body() = "GET response";
            return res;
        });

        router.post("/resource", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::created);
            res.body() = "POST response";
            return res;
        });

        auto [is_trie1, handler1, params1] = router.resolve(GET, "/resource");
        REQUIRE(handler1 != nullptr);
        
        auto [is_trie2, handler2, params2] = router.resolve(POST, "/resource");
        REQUIRE(handler2 != nullptr);
    }

    SECTION("Route not found returns nullptr") {
        router.get("/exists", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            return res;
        });

        auto [is_trie, handler, params] = router.resolve(GET, "/not-exists");
        REQUIRE(handler == nullptr);
        REQUIRE_FALSE(is_trie);
        REQUIRE(params.empty());
    }
}

TEST_CASE("Wolf Router - Complex routing scenarios", "[wolf_router]") {
    wolf_router router;

    SECTION("Nested parameterized routes") {
        router.get("/api/:version/users/:userId/posts/:postId", 
            [](const http_request& /*req*/) {
                response_t res;
                res.result(http::status::ok);
                res.body() = "Nested route";
                return res;
            });

        auto [is_trie, handler, params] = router.resolve(GET, "/api/v1/users/123/posts/456");
        REQUIRE(is_trie);
        REQUIRE(handler != nullptr);
        REQUIRE(params.size() == 3);
        REQUIRE(params.at("version") == "v1");
        REQUIRE(params.at("userId") == "123");
        REQUIRE(params.at("postId") == "456");
    }

    SECTION("Mixed static and dynamic routes") {
        router.get("/api/health", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            res.body() = "Healthy";
            return res;
        });

        router.get("/api/:resource", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            res.body() = "Dynamic resource";
            return res;
        });

        // Static route should be found
        auto [is_trie1, handler1, params1] = router.resolve(GET, "/api/health");
        REQUIRE_FALSE(is_trie1);
        REQUIRE(handler1 != nullptr);

        // Dynamic route should be found
        auto [is_trie2, handler2, params2] = router.resolve(GET, "/api/users");
        REQUIRE(is_trie2);
        REQUIRE(handler2 != nullptr);
        REQUIRE(params2.at("resource") == "users");
    }

    SECTION("RESTful API pattern") {
        router.get("/api/users", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            res.body() = R"([{"id":1,"name":"User1"}])";
            return res;
        });

        router.post("/api/users", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::created);
            res.body() = R"({"id":2,"name":"User2"})";
            return res;
        });

        router.get("/api/users/:id", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            res.body() = R"({"id":1,"name":"User1"})";
            return res;
        });

        router.put("/api/users/:id", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            res.body() = R"({"id":1,"name":"UpdatedUser"})";
            return res;
        });

        router.del("/api/users/:id", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::no_content);
            return res;
        });

        // Test all CRUD operations
        auto [is_trie_list, handler_list, params_list] = router.resolve(GET, "/api/users");
        REQUIRE(handler_list != nullptr);

        auto [is_trie_create, handler_create, params_create] = router.resolve(POST, "/api/users");
        REQUIRE(handler_create != nullptr);

        auto [is_trie_get, handler_get, params_get] = router.resolve(GET, "/api/users/1");
        REQUIRE(handler_get != nullptr);
        REQUIRE(params_get.at("id") == "1");

        auto [is_trie_update, handler_update, params_update] = router.resolve(PUT, "/api/users/1");
        REQUIRE(handler_update != nullptr);
        REQUIRE(params_update.at("id") == "1");

        auto [is_trie_delete, handler_delete, params_delete] = router.resolve(DELETE, "/api/users/1");
        REQUIRE(handler_delete != nullptr);
        REQUIRE(params_delete.at("id") == "1");
    }
}

TEST_CASE("Response building", "[response]") {
    SECTION("Basic response") {
        response_t res;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"status":"ok"})";
        res.prepare_payload();

        REQUIRE(res.result() == http::status::ok);
        REQUIRE(res[http::field::content_type] == "application/json");
        REQUIRE(res.body() == R"({"status":"ok"})");
    }

    SECTION("Error response") {
        response_t res;
        res.result(http::status::not_found);
        res.body() = "404 Not Found";
        res.prepare_payload();

        REQUIRE(res.result() == http::status::not_found);
        REQUIRE(res.body() == "404 Not Found");
    }

    SECTION("Redirect response") {
        response_t res;
        res.result(http::status::moved_permanently);
        res.set(http::field::location, "/new-location");
        res.prepare_payload();

        REQUIRE(res.result() == http::status::moved_permanently);
        REQUIRE(res[http::field::location] == "/new-location");
    }
}

TEST_CASE("HTTP Status Codes", "[status_codes]") {
    SECTION("Success codes") {
        REQUIRE(static_cast<unsigned>(http::status::ok) == 200);
        REQUIRE(static_cast<unsigned>(http::status::created) == 201);
        REQUIRE(static_cast<unsigned>(http::status::accepted) == 202);
        REQUIRE(static_cast<unsigned>(http::status::no_content) == 204);
    }

    SECTION("Client error codes") {
        REQUIRE(static_cast<unsigned>(http::status::bad_request) == 400);
        REQUIRE(static_cast<unsigned>(http::status::unauthorized) == 401);
        REQUIRE(static_cast<unsigned>(http::status::forbidden) == 403);
        REQUIRE(static_cast<unsigned>(http::status::not_found) == 404);
    }

    SECTION("Server error codes") {
        REQUIRE(static_cast<unsigned>(http::status::internal_server_error) == 500);
        REQUIRE(static_cast<unsigned>(http::status::not_implemented) == 501);
        REQUIRE(static_cast<unsigned>(http::status::bad_gateway) == 502);
        REQUIRE(static_cast<unsigned>(http::status::service_unavailable) == 503);
    }
}
