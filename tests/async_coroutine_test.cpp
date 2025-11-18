#include <catch2/catch_test_macros.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include "../src/wolf.hpp"

using namespace wolf;
namespace http = boost::beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

TEST_CASE("Async Coroutine - Type traits", "[async][coroutine][traits]") {
    SECTION("is_awaitable_v trait detection") {
        // Test awaitable types
        REQUIRE(wolf::is_awaitable_v<net::awaitable<void>> == true);
        REQUIRE(wolf::is_awaitable_v<net::awaitable<int>> == true);
        REQUIRE(wolf::is_awaitable_v<net::awaitable<std::string>> == true);
        REQUIRE(wolf::is_awaitable_v<net::awaitable<wolf::http_response>> == true);
        REQUIRE(wolf::is_awaitable_v<net::awaitable<response_t>> == true);
        
        // Test non-awaitable types
        REQUIRE(wolf::is_awaitable_v<void> == false);
        REQUIRE(wolf::is_awaitable_v<int> == false);
        REQUIRE(wolf::is_awaitable_v<std::string> == false);
        REQUIRE(wolf::is_awaitable_v<wolf::http_response> == false);
        REQUIRE(wolf::is_awaitable_v<response_t> == false);
        REQUIRE(wolf::is_awaitable_v<std::function<void()>> == false);
    }

    SECTION("Awaitable concept validation") {
        // Compile-time checks using static_assert
        using awaitable_void = net::awaitable<void>;
        using awaitable_int = net::awaitable<int>;
        using awaitable_response = net::awaitable<wolf::http_response>;
        
        static_assert(wolf::Awaitable<awaitable_void>, 
                     "awaitable<void> should satisfy Awaitable concept");
        static_assert(wolf::Awaitable<awaitable_int>, 
                     "awaitable<int> should satisfy Awaitable concept");
        static_assert(wolf::Awaitable<awaitable_response>, 
                     "awaitable<http_response> should satisfy Awaitable concept");
        
        static_assert(!wolf::Awaitable<int>, 
                     "int should not satisfy Awaitable concept");
        static_assert(!wolf::Awaitable<void>, 
                     "void should not satisfy Awaitable concept");
        static_assert(!wolf::Awaitable<wolf::http_response>, 
                     "http_response should not satisfy Awaitable concept");
        
        REQUIRE(true); // Dummy assertion for test framework
    }
}

TEST_CASE("Async Coroutine - Router with async handlers", "[async][coroutine][router]") {
    SECTION("Async router accepts awaitable handlers") {
        wolf::wolf_router async_router;
        
        bool handler_called = false;
        
        // Register async handler that returns awaitable<http_response>
        async_router.get("/async-test", [&handler_called](const http_request& req) -> net::awaitable<http_response> {
            handler_called = true;
            co_return http_response(200).text("Async response");
        });

        // Verify handler was registered
        auto [is_trie, handler, params] = async_router.resolve(http_method::GET, "/async-test");
        REQUIRE(static_cast<bool>(handler));
        REQUIRE_FALSE(is_trie);
        REQUIRE(params.empty());
    }

    SECTION("Async router with parameterized routes") {
        wolf::wolf_router async_router;
        
        async_router.get("/users/:id", [](const http_request& req) -> net::awaitable<http_response> {
            auto id = req.find_uri_param("id");
            REQUIRE(id.has_value());
            boost::json::object obj;
            obj["user_id"] = id.value();
            co_return http_response(200).json(obj);
        });

        auto [is_trie, handler, params] = async_router.resolve(http_method::GET, "/users/123");
        REQUIRE(static_cast<bool>(handler));
        REQUIRE(is_trie);
        REQUIRE(params.size() == 1);
        REQUIRE(params.at("id") == "123");
    }

    SECTION("Async router with multiple HTTP methods") {
        wolf::wolf_router async_router;
        
        async_router.get("/resource", [](const http_request& req) -> net::awaitable<http_response> {
            co_return http_response(200).text("GET response");
        });

        async_router.post("/resource", [](const http_request& req) -> net::awaitable<http_response> {
            co_return http_response(201).text("POST response");
        });

        async_router.put("/resource", [](const http_request& req) -> net::awaitable<http_response> {
            co_return http_response(200).text("PUT response");
        });

        async_router.del("/resource", [](const http_request& req) -> net::awaitable<http_response> {
            co_return http_response(204).text("");
        });

        // Verify all handlers registered
        auto [is_trie_get, handler_get, params_get] = async_router.resolve(http_method::GET, "/resource");
        REQUIRE(static_cast<bool>(handler_get));

        auto [is_trie_post, handler_post, params_post] = async_router.resolve(http_method::POST, "/resource");
        REQUIRE(static_cast<bool>(handler_post));

        auto [is_trie_put, handler_put, params_put] = async_router.resolve(http_method::PUT, "/resource");
        REQUIRE(static_cast<bool>(handler_put));

        auto [is_trie_del, handler_del, params_del] = async_router.resolve(http_method::DEL, "/resource");
        REQUIRE(static_cast<bool>(handler_del));
    }

    SECTION("Async router with JSON responses") {
        wolf::wolf_router async_router;
        
        async_router.get("/api/data", [](const http_request& req) -> net::awaitable<http_response> {
            boost::json::object data;
            data["status"] = "success";
            data["message"] = "Async JSON response";
            data["timestamp"] = 1234567890;
            
            co_return http_response(200).json(data);
        });

        auto [is_trie, handler, params] = async_router.resolve(http_method::GET, "/api/data");
        REQUIRE(static_cast<bool>(handler));
    }

    SECTION("Async router with complex nested routes") {
        wolf::wolf_router async_router;
        
        async_router.get("/api/:version/users/:userId/posts/:postId", 
            [](const http_request& req) -> net::awaitable<http_response> {
                auto version = req.find_uri_param("version");
                auto userId = req.find_uri_param("userId");
                auto postId = req.find_uri_param("postId");
                
                REQUIRE(version.has_value());
                REQUIRE(userId.has_value());
                REQUIRE(postId.has_value());
                
                boost::json::object response;
                response["version"] = version.value();
                response["userId"] = userId.value();
                response["postId"] = postId.value();
                
                co_return http_response(200).json(response);
            });

        auto [is_trie, handler, params] = async_router.resolve(
            http_method::GET, "/api/v1/users/123/posts/456");
        
        REQUIRE(is_trie);
        REQUIRE(static_cast<bool>(handler));
        REQUIRE(params.size() == 3);
        REQUIRE(params.at("version") == "v1");
        REQUIRE(params.at("userId") == "123");
        REQUIRE(params.at("postId") == "456");
    }
}

TEST_CASE("Async Coroutine - Handler execution", "[async][coroutine][execution]") {
    SECTION("Simple async handler execution") {
        net::io_context ioc;
        
        std::atomic<bool> executed{false};
        
        auto async_handler = [&executed](const http_request& req) -> net::awaitable<http_response> {
            executed = true;
            co_return http_response(200).text("Executed");
        };

        // Create a simple request
        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/test");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);

        // Execute the async handler
        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req);
            REQUIRE(response.result() == http::status::ok);
            REQUIRE(response.body() == "Executed");
        }, net::detached);

        ioc.run();
        REQUIRE(executed == true);
    }

    SECTION("Async handler with delay simulation") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
            auto executor = co_await net::this_coro::executor;
            net::steady_timer timer(executor);
            timer.expires_after(std::chrono::milliseconds(10));
            co_await timer.async_wait(net::use_awaitable);
            
            co_return http_response(200).text("Delayed response");
        };

        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/delayed");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);

        bool completed = false;
        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req);
            REQUIRE(response.result() == http::status::ok);
            REQUIRE(response.body() == "Delayed response");
            completed = true;
        }, net::detached);

        ioc.run();
        REQUIRE(completed);
    }

    SECTION("Multiple concurrent async handlers") {
        net::io_context ioc;
        std::atomic<int> execution_count{0};
        
        auto async_handler = [&execution_count](int id) -> net::awaitable<http_response> {
            auto executor = co_await net::this_coro::executor;
            net::steady_timer timer(executor);
            timer.expires_after(std::chrono::milliseconds(5 * id));
            co_await timer.async_wait(net::use_awaitable);
            
            execution_count++;
            co_return http_response(200).text("Handler " + std::to_string(id));
        };

        // Spawn multiple concurrent handlers
        for (int i = 1; i <= 5; ++i) {
            net::co_spawn(ioc, [&, i]() -> net::awaitable<void> {
                auto response = co_await async_handler(i);
                REQUIRE(response.result() == http::status::ok);
            }, net::detached);
        }

        ioc.run();
        REQUIRE(execution_count == 5);
    }

    SECTION("Async handler with JSON request processing") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
            auto json_body = req.get_json_body();
            
            boost::json::object response;
            response["received"] = true;
            response["echo"] = json_body;
            
            co_return http_response(200).json(response);
        };

        http::request<http::string_body> base_req;
        base_req.method(http::verb::post);
        base_req.target("/api/echo");
        base_req.body() = R"({"message":"Hello, async world!"})";
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);

        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req);
            REQUIRE(response.result() == http::status::ok);
            REQUIRE(response[http::field::content_type] == "application/json");
            
            auto response_json = boost::json::parse(response.body()).as_object();
            REQUIRE(response_json.at("received").as_bool() == true);
        }, net::detached);

        ioc.run();
    }

    SECTION("Async handler with error responses") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
            auto id = req.find_uri_param("id");
            
            if (!id.has_value() || id.value() == "0") {
                boost::json::object error_obj;
                error_obj["error"] = "Invalid ID";
                error_obj["code"] = 400;
                co_return http_response(400).json(error_obj);
            }
            
            boost::json::object success_obj;
            success_obj["id"] = id.value();
            success_obj["status"] = "found";
            co_return http_response(200).json(success_obj);
        };

        // Test with invalid ID
        http::request<http::string_body> bad_req;
        bad_req.method(http::verb::get);
        bad_req.target("/users/0");
        
        params_t bad_params;
        bad_params["id"] = "0";
        auto query_result = boost::urls::parse_query("");
        http_request req_bad(bad_req, bad_params, query_result);

        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req_bad);
            REQUIRE(response.result() == http::status::bad_request);
        }, net::detached);

        // Test with valid ID
        http::request<http::string_body> good_req;
        good_req.method(http::verb::get);
        good_req.target("/users/123");
        
        params_t good_params;
        good_params["id"] = "123";
        http_request req_good(good_req, good_params, query_result);

        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req_good);
            REQUIRE(response.result() == http::status::ok);
        }, net::detached);

        ioc.run();
    }
}

TEST_CASE("Async Coroutine - Fluent API integration", "[async][coroutine][fluent]") {
    SECTION("Async handler with chained fluent API") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
            boost::json::object obj;
            obj["created"] = true;
            obj["id"] = 42;
            co_return http_response(201)
                .header("X-Custom-Header", "AsyncValue")
                .cookie("session_id", "async123")
                .json(obj);
        };

        http::request<http::string_body> base_req;
        base_req.method(http::verb::post);
        base_req.target("/create");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);

        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req);
            
            REQUIRE(response.result() == http::status::created);
            REQUIRE(response[http::field::content_type] == "application/json");
            REQUIRE(response["X-Custom-Header"] == "AsyncValue");
            REQUIRE(std::string(response[http::field::set_cookie]).find("session_id=async123") != std::string::npos);
            
            auto json_body = boost::json::parse(response.body()).as_object();
            REQUIRE(json_body.at("created").as_bool() == true);
            REQUIRE(json_body.at("id").as_int64() == 42);
        }, net::detached);

        ioc.run();
    }

    SECTION("Async handler with HTML response") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
            co_return http_response(200).html("<h1>Async HTML</h1>");
        };

        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/page");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);

        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req);
            
            REQUIRE(response.result() == http::status::ok);
            REQUIRE(response[http::field::content_type] == "text/html");
            REQUIRE(response.body() == "<h1>Async HTML</h1>");
        }, net::detached);

        ioc.run();
    }

    SECTION("Async handler with status chaining") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
            boost::json::object obj;
            obj["error"] = "Not found";
            obj["path"] = std::string(req.target());
            co_return http_response()
                .status(404)
                .json(obj);
        };

        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/missing");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);

        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req);
            
            REQUIRE(response.result() == http::status::not_found);
            
            auto json_body = boost::json::parse(response.body()).as_object();
            REQUIRE(json_body.at("error").as_string() == "Not found");
            REQUIRE(json_body.at("path").as_string() == "/missing");
        }, net::detached);

        ioc.run();
    }
}

TEST_CASE("Async Coroutine - RESTful API patterns", "[async][coroutine][rest]") {
    SECTION("Async CRUD operations") {
        wolf::wolf_router async_router;
        
        // CREATE
        async_router.post("/api/users", [](const http_request& req) -> net::awaitable<http_response> {
            auto json_body = req.get_json_body();
            boost::json::object obj;
            obj["id"] = 1;
            obj["name"] = json_body.at("name");
            obj["created"] = true;
            co_return http_response(201).json(obj);
        });

        // READ (list)
        async_router.get("/api/users", [](const http_request& req) -> net::awaitable<http_response> {
            boost::json::array users;
            users.push_back({{"id", 1}, {"name", "User1"}});
            users.push_back({{"id", 2}, {"name", "User2"}});
            co_return http_response(200).json(users);
        });

        // READ (single)
        async_router.get("/api/users/:id", [](const http_request& req) -> net::awaitable<http_response> {
            auto id = req.find_uri_param("id").value_or("0");
            boost::json::object obj;
            obj["id"] = id;
            obj["name"] = "User" + id;
            co_return http_response(200).json(obj);
        });

        // UPDATE
        async_router.put("/api/users/:id", [](const http_request& req) -> net::awaitable<http_response> {
            auto id = req.find_uri_param("id").value_or("0");
            auto json_body = req.get_json_body();
            boost::json::object obj;
            obj["id"] = id;
            obj["name"] = json_body.at("name");
            obj["updated"] = true;
            co_return http_response(200).json(obj);
        });

        // DELETE
        async_router.del("/api/users/:id", [](const http_request& req) -> net::awaitable<http_response> {
            co_return http_response(204).text("");
        });

        // Verify all routes registered
        auto [is_trie_post, handler_post, params_post] = async_router.resolve(http_method::POST, "/api/users");
        REQUIRE(static_cast<bool>(handler_post));

        auto [is_trie_get_list, handler_get_list, params_get_list] = async_router.resolve(http_method::GET, "/api/users");
        REQUIRE(static_cast<bool>(handler_get_list));

        auto [is_trie_get, handler_get, params_get] = async_router.resolve(http_method::GET, "/api/users/1");
        REQUIRE(static_cast<bool>(handler_get));

        auto [is_trie_put, handler_put, params_put] = async_router.resolve(http_method::PUT, "/api/users/1");
        REQUIRE(static_cast<bool>(handler_put));

        auto [is_trie_del, handler_del, params_del] = async_router.resolve(http_method::DEL, "/api/users/1");
        REQUIRE(static_cast<bool>(handler_del));
    }
}

TEST_CASE("Async Coroutine - Error handling", "[async][coroutine][errors]") {
    SECTION("Async handler with exception handling") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req, bool throw_error) -> net::awaitable<http_response> {
            if (throw_error) {
                // In production, you'd catch exceptions and return error responses
                boost::json::object error_obj;
                error_obj["error"] = "Internal server error";
                error_obj["message"] = "Something went wrong";
                co_return http_response(500).json(error_obj);
            }
            
            boost::json::object success_obj;
            success_obj["status"] = "ok";
            co_return http_response(200).json(success_obj);
        };

        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/test");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);

        // Test error case
        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req, true);
            REQUIRE(response.result() == http::status::internal_server_error);
        }, net::detached);

        // Test success case
        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req, false);
            REQUIRE(response.result() == http::status::ok);
        }, net::detached);

        ioc.run();
    }

    SECTION("Async handler timeout simulation") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
            auto executor = co_await net::this_coro::executor;
            net::steady_timer timer(executor);
            timer.expires_after(std::chrono::milliseconds(50));
            co_await timer.async_wait(net::use_awaitable);
            
            boost::json::object obj;
            obj["completed"] = true;
            co_return http_response(200).json(obj);
        };

        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/slow");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);

        bool completed = false;
        auto start = std::chrono::steady_clock::now();
        
        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req);
            auto duration = std::chrono::steady_clock::now() - start;
            
            REQUIRE(response.result() == http::status::ok);
            REQUIRE(duration >= std::chrono::milliseconds(50));
            completed = true;
        }, net::detached);

        ioc.run();
        REQUIRE(completed);
    }
}

TEST_CASE("Async Coroutine - Query and cookie handling", "[async][coroutine][params]") {
    SECTION("Async handler with query parameters") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
            auto page = req.find_query_param("page").value_or("1");
            auto limit = req.find_query_param("limit").value_or("10");
            
            boost::json::object obj;
            obj["page"] = page;
            obj["limit"] = limit;
            co_return http_response(200).json(obj);
        };

        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/items?page=2&limit=20");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("page=2&limit=20");
        http_request req(base_req, uri_params, query_result);

        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req);
            
            REQUIRE(response.result() == http::status::ok);
            auto json_body = boost::json::parse(response.body()).as_object();
            REQUIRE(json_body.at("page").as_string() == "2");
            REQUIRE(json_body.at("limit").as_string() == "20");
        }, net::detached);

        ioc.run();
    }

    SECTION("Async handler with cookies") {
        net::io_context ioc;
        
        auto async_handler = [](const http_request& req) -> net::awaitable<http_response> {
            auto session_id = req.find_cookie("session_id").value_or("none");
            
            boost::json::object obj;
            obj["session_id"] = session_id;
            co_return http_response(200)
                .json(obj)
                .cookie("new_token", "async_token_123");
        };

        http::request<http::string_body> base_req;
        base_req.method(http::verb::get);
        base_req.target("/auth");
        base_req.set(http::field::cookie, "session_id=abc123");
        
        params_t uri_params;
        auto query_result = boost::urls::parse_query("");
        http_request req(base_req, uri_params, query_result);

        net::co_spawn(ioc, [&]() -> net::awaitable<void> {
            auto response = co_await async_handler(req);
            
            REQUIRE(response.result() == http::status::ok);
            
            auto json_body = boost::json::parse(response.body()).as_object();
            REQUIRE(json_body.at("session_id").as_string() == "abc123");
            
            auto cookie_header = std::string(response[http::field::set_cookie]);
            REQUIRE(cookie_header.find("new_token=async_token_123") != std::string::npos);
        }, net::detached);

        ioc.run();
    }
}
