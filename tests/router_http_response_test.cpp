#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "../src/wolf.hpp"
#include <boost/json.hpp>

namespace json = boost::json;

TEST_CASE("http_router - http_response return type", "[http_router][http_response]") {
    
    wolf::wolf_router router;

    SECTION("GET handler returning http_response with json") {
        router.get("/api/test", [](const wolf::http_request& req) -> wolf::http_response {
            json::object data = {{"message", "success"}, {"code", 200}};
            return wolf::http_response(200).json(data);
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::GET, "/api/test");
        
        REQUIRE(handler != nullptr);
        REQUIRE_FALSE(is_trie);

        // Create a mock request
        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::get);
        base_req.target("/api/test");
        wolf::http_request req(base_req, {}, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::ok);
        REQUIRE(response[boost::beast::http::field::content_type] == "application/json");
        
        auto parsed = json::parse(response.body());
        REQUIRE(parsed.as_object()["message"].as_string() == "success");
        REQUIRE(parsed.as_object()["code"].as_int64() == 200);
    }

    SECTION("POST handler returning http_response with status 201") {
        router.post("/api/users", [](const wolf::http_request& req) -> wolf::http_response {
            json::object result = {{"id", 123}, {"created", true}};
            return wolf::http_response(201).json(result);
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::POST, "/api/users");
        
        REQUIRE(handler != nullptr);

        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::post);
        base_req.target("/api/users");
        wolf::http_request req(base_req, {}, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::created);
        REQUIRE(response[boost::beast::http::field::content_type] == "application/json");
    }

    SECTION("PUT handler returning http_response") {
        router.put("/api/resource/:id", [](const wolf::http_request& req) -> wolf::http_response {
            auto id = req.get_or("id", "0");
            json::object result = {{"updated", true}, {"id", id}};
            return wolf::http_response(200).json(result);
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::PUT, "/api/resource/42");
        
        REQUIRE(handler != nullptr);
        REQUIRE(is_trie);
        REQUIRE(params["id"] == "42");

        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::put);
        base_req.target("/api/resource/42");
        wolf::http_request req(base_req, params, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::ok);
    }

    SECTION("DELETE handler returning http_response with 204") {
        router.del("/api/items/:id", [](const wolf::http_request& req) -> wolf::http_response {
            return wolf::http_response(204).text("");
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::DEL, "/api/items/99");
        
        REQUIRE(handler != nullptr);
        REQUIRE(is_trie);
        REQUIRE(params["id"] == "99");

        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::delete_);
        base_req.target("/api/items/99");
        wolf::http_request req(base_req, params, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::no_content);
    }

    SECTION("Handler returning http_response with custom headers") {
        router.get("/api/info", [](const wolf::http_request& req) -> wolf::http_response {
            json::object data = {{"info", "test"}};
            return wolf::http_response(200)
                .header("X-Custom-Header", "CustomValue")
                .header("X-API-Version", "1.0")
                .json(data);
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::GET, "/api/info");
        
        REQUIRE(handler != nullptr);

        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::get);
        base_req.target("/api/info");
        wolf::http_request req(base_req, {}, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::ok);
        REQUIRE(response["X-Custom-Header"] == "CustomValue");
        REQUIRE(response["X-API-Version"] == "1.0");
        REQUIRE(response[boost::beast::http::field::content_type] == "application/json");
    }

    SECTION("Handler returning http_response with cookie") {
        router.post("/api/login", [](const wolf::http_request& req) -> wolf::http_response {
            json::object result = {{"success", true}};
            return wolf::http_response(200)
                .json(result)
                .cookie("session", "abc123", "/", "", 3600, true, false);
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::POST, "/api/login");
        
        REQUIRE(handler != nullptr);

        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::post);
        base_req.target("/api/login");
        wolf::http_request req(base_req, {}, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::ok);
        
        std::string cookie = std::string(response[boost::beast::http::field::set_cookie]);
        REQUIRE(cookie.find("session=abc123") != std::string::npos);
        REQUIRE(cookie.find("Max-Age=3600") != std::string::npos);
    }

    SECTION("Handler returning http_response with text") {
        router.get("/health", [](const wolf::http_request& req) -> wolf::http_response {
            return wolf::http_response(200).text("OK");
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::GET, "/health");
        
        REQUIRE(handler != nullptr);

        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::get);
        base_req.target("/health");
        wolf::http_request req(base_req, {}, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::ok);
        REQUIRE(response[boost::beast::http::field::content_type] == "text/plain");
        REQUIRE(response.body() == "OK");
    }

    SECTION("Handler returning http_response with HTML") {
        router.get("/page", [](const wolf::http_request& req) -> wolf::http_response {
            return wolf::http_response(200).html("<h1>Hello</h1>");
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::GET, "/page");
        
        REQUIRE(handler != nullptr);

        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::get);
        base_req.target("/page");
        wolf::http_request req(base_req, {}, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::ok);
        REQUIRE(response[boost::beast::http::field::content_type] == "text/html");
        REQUIRE(response.body() == "<h1>Hello</h1>");
    }

    SECTION("Handler returning http_response with error status") {
        router.get("/api/error", [](const wolf::http_request& req) -> wolf::http_response {
            json::object error = {
                {"error", "Not Found"},
                {"message", "Resource not found"},
                {"code", 404}
            };
            return wolf::http_response(404).json(error);
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::GET, "/api/error");
        
        REQUIRE(handler != nullptr);

        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::get);
        base_req.target("/api/error");
        wolf::http_request req(base_req, {}, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::not_found);
        REQUIRE(response[boost::beast::http::field::content_type] == "application/json");
        
        auto parsed = json::parse(response.body());
        REQUIRE(parsed.as_object()["error"].as_string() == "Not Found");
        REQUIRE(parsed.as_object()["code"].as_int64() == 404);
    }

    SECTION("Multiple HTTP methods with http_response") {
        router.get("/api/resource", [](const wolf::http_request& req) -> wolf::http_response {
            return wolf::http_response(200).json(json::object{{"method", "GET"}});
        });

        router.post("/api/resource", [](const wolf::http_request& req) -> wolf::http_response {
            return wolf::http_response(201).json(json::object{{"method", "POST"}});
        });

        router.patch("/api/resource", [](const wolf::http_request& req) -> wolf::http_response {
            return wolf::http_response(200).json(json::object{{"method", "PATCH"}});
        });

        // Test GET
        {
            auto [is_trie, handler, params] = router.resolve(wolf::http_method::GET, "/api/resource");
            REQUIRE(handler != nullptr);
            
            wolf::request_t base_req;
            base_req.method(boost::beast::http::verb::get);
            base_req.target("/api/resource");
            wolf::http_request req(base_req, {}, boost::urls::parse_query(""));
            
            auto response = handler(req);
            REQUIRE(response.result() == boost::beast::http::status::ok);
            
            auto parsed = json::parse(response.body());
            REQUIRE(parsed.as_object()["method"].as_string() == "GET");
        }

        // Test POST
        {
            auto [is_trie, handler, params] = router.resolve(wolf::http_method::POST, "/api/resource");
            REQUIRE(handler != nullptr);
            
            wolf::request_t base_req;
            base_req.method(boost::beast::http::verb::post);
            base_req.target("/api/resource");
            wolf::http_request req(base_req, {}, boost::urls::parse_query(""));
            
            auto response = handler(req);
            REQUIRE(response.result() == boost::beast::http::status::created);
            
            auto parsed = json::parse(response.body());
            REQUIRE(parsed.as_object()["method"].as_string() == "POST");
        }

        // Test PATCH
        {
            auto [is_trie, handler, params] = router.resolve(wolf::http_method::PATCH, "/api/resource");
            REQUIRE(handler != nullptr);
            
            wolf::request_t base_req;
            base_req.method(boost::beast::http::verb::patch);
            base_req.target("/api/resource");
            wolf::http_request req(base_req, {}, boost::urls::parse_query(""));
            
            auto response = handler(req);
            REQUIRE(response.result() == boost::beast::http::status::ok);
            
            auto parsed = json::parse(response.body());
            REQUIRE(parsed.as_object()["method"].as_string() == "PATCH");
        }
    }

    SECTION("Complex chaining with http_response") {
        router.post("/api/complex", [](const wolf::http_request& req) -> wolf::http_response {
            json::object data = {
                {"status", "success"},
                {"timestamp", std::time(nullptr)}
            };
            
            return wolf::http_response(200)
                .header("X-Request-ID", "12345")
                .header("X-API-Version", "2.0")
                .header("X-RateLimit-Limit", "1000")
                .json(data)
                .cookie("tracking", "xyz789", "/", "", 86400, false, false);
        });

        auto [is_trie, handler, params] = router.resolve(wolf::http_method::POST, "/api/complex");
        
        REQUIRE(handler != nullptr);

        wolf::request_t base_req;
        base_req.method(boost::beast::http::verb::post);
        base_req.target("/api/complex");
        wolf::http_request req(base_req, {}, boost::urls::parse_query(""));

        auto response = handler(req);
        
        REQUIRE(response.result() == boost::beast::http::status::ok);
        REQUIRE(response["X-Request-ID"] == "12345");
        REQUIRE(response["X-API-Version"] == "2.0");
        REQUIRE(response["X-RateLimit-Limit"] == "1000");
        REQUIRE(response[boost::beast::http::field::content_type] == "application/json");
        
        std::string cookie = std::string(response[boost::beast::http::field::set_cookie]);
        REQUIRE(cookie.find("tracking=xyz789") != std::string::npos);
    }
}
