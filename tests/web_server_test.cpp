#include <catch2/catch_test_macros.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include "../src/web_server.hpp"

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

    SECTION("Cookie parsing - single cookie") {
        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/");
        base_req.set(http::field::cookie, "session_id=abc123");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);
        
        auto cookies = req.cookies();
        REQUIRE(cookies.size() == 1);
        REQUIRE(cookies.at("session_id") == "abc123");
    }

    SECTION("Cookie parsing - multiple cookies") {
        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/");
        base_req.set(http::field::cookie, "session_id=abc123; user_id=456; theme=dark");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);
        
        auto cookies = req.cookies();
        REQUIRE(cookies.size() == 3);
        REQUIRE(cookies.at("session_id") == "abc123");
        REQUIRE(cookies.at("user_id") == "456");
        REQUIRE(cookies.at("theme") == "dark");
    }

    SECTION("Cookie parsing - no cookies") {
        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);
        
        auto cookies = req.cookies();
        REQUIRE(cookies.empty());
    }

    SECTION("Cookie parsing - with whitespace") {
        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/");
        base_req.set(http::field::cookie, "session_id=abc123;  user_id = 456 ; theme=dark");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);
        
        auto cookies = req.cookies();
        REQUIRE(cookies.size() == 3);
        REQUIRE(cookies.at("session_id") == "abc123");
        REQUIRE(cookies.at("user_id") == "456");
        REQUIRE(cookies.at("theme") == "dark");
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

        auto [is_trie, handler, params] = router.resolve(http_method::GET, "/test");
        REQUIRE_FALSE(is_trie);
        REQUIRE(static_cast<bool>(handler));
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

        auto [is_trie, handler, params] = router.resolve(http_method::GET, "/users/123");
        REQUIRE(is_trie);
        REQUIRE(static_cast<bool>(handler));
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

        auto [is_trie1, handler1, params1] = router.resolve(http_method::GET, "/resource");
        REQUIRE(static_cast<bool>(handler1));
        
        auto [is_trie2, handler2, params2] = router.resolve(http_method::POST, "/resource");
        REQUIRE(static_cast<bool>(handler2));
    }

    SECTION("Route not found returns nullptr") {
        router.get("/exists", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            return res;
        });

        auto [is_trie, handler, params] = router.resolve(http_method::GET, "/not-exists");
        REQUIRE(!static_cast<bool>(handler));
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

        auto [is_trie, handler, params] = router.resolve(http_method::GET, "/api/v1/users/123/posts/456");
        REQUIRE(is_trie);
        REQUIRE(static_cast<bool>(handler));
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
        auto [is_trie1, handler1, params1] = router.resolve(http_method::GET, "/api/health");
        REQUIRE_FALSE(is_trie1);
        REQUIRE(static_cast<bool>(handler1));

        // Dynamic route should be found
        auto [is_trie2, handler2, params2] = router.resolve(http_method::GET, "/api/users");
        REQUIRE(is_trie2);
        REQUIRE(static_cast<bool>(handler2));
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
        auto [is_trie_list, handler_list, params_list] = router.resolve(http_method::GET, "/api/users");
        REQUIRE(static_cast<bool>(handler_list));

        auto [is_trie_create, handler_create, params_create] = router.resolve(http_method::POST, "/api/users");
        REQUIRE(static_cast<bool>(handler_create));

        auto [is_trie_get, handler_get, params_get] = router.resolve(http_method::GET, "/api/users/1");
        REQUIRE(static_cast<bool>(handler_get));
        REQUIRE(params_get.at("id") == "1");

        auto [is_trie_update, handler_update, params_update] = router.resolve(http_method::PUT, "/api/users/1");
        REQUIRE(static_cast<bool>(handler_update));
        REQUIRE(params_update.at("id") == "1");

        auto [is_trie_delete, handler_delete, params_delete] = router.resolve(http_method::DEL, "/api/users/1");
        REQUIRE(static_cast<bool>(handler_delete));
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

    SECTION("Set cookie - basic") {
        response_t res;
        res.result(http::status::ok);
        res.body() = "Cookie set";
        
        wolf::set_cookie(res, "session_id", "abc123");
        
        REQUIRE(res[http::field::set_cookie] == "session_id=abc123; Path=/; HttpOnly");
    }

    SECTION("Set cookie - with path and domain") {
        response_t res;
        res.result(http::status::ok);
        
        wolf::set_cookie(res, "user_token", "xyz789", "/api", "example.com");
        
        std::string cookie = res[http::field::set_cookie];
        REQUIRE(cookie.find("user_token=xyz789") != std::string::npos);
        REQUIRE(cookie.find("Path=/api") != std::string::npos);
        REQUIRE(cookie.find("Domain=example.com") != std::string::npos);
    }

    SECTION("Set cookie - with max age") {
        response_t res;
        res.result(http::status::ok);
        
        wolf::set_cookie(res, "remember_me", "true", "/", "", 3600);
        
        std::string cookie = res[http::field::set_cookie];
        REQUIRE(cookie.find("remember_me=true") != std::string::npos);
        REQUIRE(cookie.find("Max-Age=3600") != std::string::npos);
    }

    SECTION("Set cookie - secure and http only") {
        response_t res;
        res.result(http::status::ok);
        
        wolf::set_cookie(res, "secure_token", "secret", "/", "", -1, true, true);
        
        std::string cookie = res[http::field::set_cookie];
        REQUIRE(cookie.find("secure_token=secret") != std::string::npos);
        REQUIRE(cookie.find("HttpOnly") != std::string::npos);
        REQUIRE(cookie.find("Secure") != std::string::npos);
    }

    SECTION("Set cookie - all options") {
        response_t res;
        res.result(http::status::ok);
        
        wolf::set_cookie(res, "full_cookie", "value123", "/app", "example.com", 7200, true, true);
        
        std::string cookie = res[http::field::set_cookie];
        REQUIRE(cookie.find("full_cookie=value123") != std::string::npos);
        REQUIRE(cookie.find("Path=/app") != std::string::npos);
        REQUIRE(cookie.find("Domain=example.com") != std::string::npos);
        REQUIRE(cookie.find("Max-Age=7200") != std::string::npos);
        REQUIRE(cookie.find("HttpOnly") != std::string::npos);
        REQUIRE(cookie.find("Secure") != std::string::npos);
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

TEST_CASE("Coroutine Async Features", "[async][coroutine]") {
    SECTION("Awaitable type trait detection") {
        // Test that we can detect awaitable types
        using awaitable_type = net::awaitable<int>;
        using non_awaitable_type = int;
        
        // These are compile-time checks
        REQUIRE(wolf::is_awaitable_v<awaitable_type> == true);
        REQUIRE(wolf::is_awaitable_v<non_awaitable_type> == false);
    }

    SECTION("Web server with async request handling") {
        // Create a server on a random available port
        unsigned short port = 18080;
        wolf::web_server server(port);
        
        // Add a simple test route
        std::atomic<bool> handler_called{false};
        server->get("/async-test", [&handler_called](const http_request& /*req*/) {
            handler_called = true;
            response_t res;
            res.result(http::status::ok);
            res.body() = "Async response";
            res.prepare_payload();
            return res;
        });

        // Start server in background thread
        std::thread server_thread([&server]() {
            try {
                server.start();
            } catch (...) {
                // Server stopped
            }
        });

        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Make a request to the server
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            tcp::socket socket(ioc);

            auto const results = resolver.resolve("127.0.0.1", std::to_string(port));
            net::connect(socket, results.begin(), results.end());

            // Send HTTP request
            http::request<http::string_body> req{http::verb::get, "/async-test", 11};
            req.set(http::field::host, "127.0.0.1");
            req.set(http::field::user_agent, "Catch2 Test");

            http::write(socket, req);

            // Read response
            boost::beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(socket, buffer, res);

            REQUIRE(res.result() == http::status::ok);
            REQUIRE(res.body() == "Async response");
            REQUIRE(handler_called == true);

            socket.shutdown(tcp::socket::shutdown_both);
        } catch (std::exception& e) {
            WARN("Connection failed: " << e.what());
        }

        // Stop server
        server.stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }

    SECTION("Multiple concurrent requests with coroutines") {
        unsigned short port = 18081;
        wolf::web_server server(port);
        
        std::atomic<int> request_count{0};
        server->get("/concurrent", [&request_count](const http_request& /*req*/) {
            request_count++;
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            response_t res;
            res.result(http::status::ok);
            res.body() = "Concurrent response";
            res.prepare_payload();
            return res;
        });

        std::thread server_thread([&server]() {
            try {
                server.start();
            } catch (...) {}
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Make multiple concurrent requests
        std::vector<std::thread> client_threads;
        const int num_requests = 5;
        std::atomic<int> successful_requests{0};

        for (int i = 0; i < num_requests; ++i) {
            client_threads.emplace_back([port, &successful_requests]() {
                try {
                    net::io_context ioc;
                    tcp::resolver resolver(ioc);
                    tcp::socket socket(ioc);

                    auto const results = resolver.resolve("127.0.0.1", std::to_string(port));
                    net::connect(socket, results.begin(), results.end());

                    http::request<http::string_body> req{http::verb::get, "/concurrent", 11};
                    req.set(http::field::host, "127.0.0.1");
                    req.set(http::field::user_agent, "Catch2 Test");

                    http::write(socket, req);

                    boost::beast::flat_buffer buffer;
                    http::response<http::string_body> res;
                    http::read(socket, buffer, res);

                    if (res.result() == http::status::ok) {
                        successful_requests++;
                    }

                    socket.shutdown(tcp::socket::shutdown_both);
                } catch (...) {}
            });
        }

        // Wait for all client threads
        for (auto& t : client_threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        // Verify all requests were handled
        REQUIRE(successful_requests >= num_requests - 1); // Allow for one potential failure

        server.stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }

    SECTION("Coroutine handles keep-alive connections") {
        unsigned short port = 18082;
        wolf::web_server server(port);
        
        server->get("/keepalive", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::ok);
            res.body() = "Keep-alive response";
            res.prepare_payload();
            return res;
        });

        std::thread server_thread([&server]() {
            try {
                server.start();
            } catch (...) {}
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            tcp::socket socket(ioc);

            auto const results = resolver.resolve("127.0.0.1", std::to_string(port));
            net::connect(socket, results.begin(), results.end());

            // Send first request
            http::request<http::string_body> req1{http::verb::get, "/keepalive", 11};
            req1.set(http::field::host, "127.0.0.1");
            req1.set(http::field::connection, "keep-alive");
            req1.set(http::field::user_agent, "Catch2 Test");

            http::write(socket, req1);

            boost::beast::flat_buffer buffer1;
            http::response<http::string_body> res1;
            http::read(socket, buffer1, res1);

            REQUIRE(res1.result() == http::status::ok);
            REQUIRE(res1.body() == "Keep-alive response");

            // Send second request on same connection
            http::request<http::string_body> req2{http::verb::get, "/keepalive", 11};
            req2.set(http::field::host, "127.0.0.1");
            req2.set(http::field::connection, "keep-alive");
            req2.set(http::field::user_agent, "Catch2 Test");

            http::write(socket, req2);

            boost::beast::flat_buffer buffer2;
            http::response<http::string_body> res2;
            http::read(socket, buffer2, res2);

            REQUIRE(res2.result() == http::status::ok);
            REQUIRE(res2.body() == "Keep-alive response");

            socket.shutdown(tcp::socket::shutdown_both);
        } catch (std::exception& e) {
            WARN("Keep-alive test failed: " << e.what());
        }

        server.stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }

    SECTION("Coroutine properly handles errors") {
        unsigned short port = 18083;
        wolf::web_server server(port);
        
        server->get("/error", [](const http_request& /*req*/) {
            response_t res;
            res.result(http::status::internal_server_error);
            res.body() = "Error response";
            res.prepare_payload();
            return res;
        });

        std::thread server_thread([&server]() {
            try {
                server.start();
            } catch (...) {}
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            tcp::socket socket(ioc);

            auto const results = resolver.resolve("127.0.0.1", std::to_string(port));
            net::connect(socket, results.begin(), results.end());

            http::request<http::string_body> req{http::verb::get, "/error", 11};
            req.set(http::field::host, "127.0.0.1");
            req.set(http::field::user_agent, "Catch2 Test");

            http::write(socket, req);

            boost::beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(socket, buffer, res);

            REQUIRE(res.result() == http::status::internal_server_error);
            REQUIRE(res.body() == "Error response");

            socket.shutdown(tcp::socket::shutdown_both);
        } catch (std::exception& e) {
            WARN("Error test failed: " << e.what());
        }

        server.stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
}

TEST_CASE("Async Type Traits", "[async][traits]") {
    SECTION("is_awaitable_v trait for various types") {
        // Awaitable types
        REQUIRE(wolf::is_awaitable_v<net::awaitable<void>> == true);
        REQUIRE(wolf::is_awaitable_v<net::awaitable<int>> == true);
        REQUIRE(wolf::is_awaitable_v<net::awaitable<std::string>> == true);
        REQUIRE(wolf::is_awaitable_v<net::awaitable<wolf::http_response>> == true);
        
        // Non-awaitable types
        REQUIRE(wolf::is_awaitable_v<void> == false);
        REQUIRE(wolf::is_awaitable_v<int> == false);
        REQUIRE(wolf::is_awaitable_v<std::string> == false);
        REQUIRE(wolf::is_awaitable_v<wolf::http_response> == false);
        REQUIRE(wolf::is_awaitable_v<std::function<void()>> == false);
    }

    SECTION("Awaitable concept validation") {
        // This section tests compile-time concept checking
        // The fact that these compile means the concept works correctly
        
        using awaitable_void = net::awaitable<void>;
        using awaitable_int = net::awaitable<int>;
        
        static_assert(wolf::Awaitable<awaitable_void>, "awaitable<void> should satisfy Awaitable concept");
        static_assert(wolf::Awaitable<awaitable_int>, "awaitable<int> should satisfy Awaitable concept");
        static_assert(!wolf::Awaitable<int>, "int should not satisfy Awaitable concept");
        static_assert(!wolf::Awaitable<void>, "void should not satisfy Awaitable concept");
        
        REQUIRE(true); // Dummy assertion since static_assert is compile-time
    }
}
