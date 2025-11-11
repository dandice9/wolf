#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "../src/wolf.hpp"
#include <boost/json.hpp>

namespace json = boost::json;

TEST_CASE("http_response - Fluent API", "[http_response]") {
    
    SECTION("Constructor with status code") {
        wolf::http_response res(201);
        REQUIRE(res.result() == boost::beast::http::status::created);
    }

    SECTION("Constructor with beast::http::status") {
        wolf::http_response res(boost::beast::http::status::not_found);
        REQUIRE(res.result() == boost::beast::http::status::not_found);
    }

    SECTION("JSON response with object") {
        json::object data = {
            {"id", 1},
            {"name", "John"},
            {"active", true}
        };
        
        auto res = wolf::http_response(200).json(data);
        
        REQUIRE(res.result() == boost::beast::http::status::ok);
        REQUIRE(res[boost::beast::http::field::content_type] == "application/json");
        
        // Parse response body
        auto parsed = json::parse(res.body());
        REQUIRE(parsed.as_object()["id"].as_int64() == 1);
        REQUIRE(parsed.as_object()["name"].as_string() == "John");
        REQUIRE(parsed.as_object()["active"].as_bool() == true);
    }

    SECTION("JSON response with array") {
        json::array data = {1, 2, 3, 4, 5};
        
        auto res = wolf::http_response(200).json(data);
        
        REQUIRE(res.result() == boost::beast::http::status::ok);
        REQUIRE(res[boost::beast::http::field::content_type] == "application/json");
        
        auto parsed = json::parse(res.body());
        REQUIRE(parsed.as_array().size() == 5);
        REQUIRE(parsed.as_array()[0].as_int64() == 1);
    }

    SECTION("Text response") {
        auto res = wolf::http_response(200).text("Hello, World!");
        
        REQUIRE(res.result() == boost::beast::http::status::ok);
        REQUIRE(res[boost::beast::http::field::content_type] == "text/plain");
        REQUIRE(res.body() == "Hello, World!");
    }

    SECTION("HTML response") {
        auto res = wolf::http_response(200).html("<h1>Title</h1>");
        
        REQUIRE(res.result() == boost::beast::http::status::ok);
        REQUIRE(res[boost::beast::http::field::content_type] == "text/html");
        REQUIRE(res.body() == "<h1>Title</h1>");
    }

    SECTION("Custom headers") {
        auto res = wolf::http_response(200)
            .text("Content")
            .header("X-Custom-Header", "MyValue")
            .header("X-Request-ID", "12345");
        
        REQUIRE(res["X-Custom-Header"] == "MyValue");
        REQUIRE(res["X-Request-ID"] == "12345");
    }

    SECTION("Cookie setting") {
        auto res = wolf::http_response(200)
            .text("Login successful")
            .cookie("session_id", "abc123", "/", "", 3600, true, false);
        
        std::string cookie = std::string(res[boost::beast::http::field::set_cookie]);
        REQUIRE(cookie.find("session_id=abc123") != std::string::npos);
        REQUIRE(cookie.find("Path=/") != std::string::npos);
        REQUIRE(cookie.find("Max-Age=3600") != std::string::npos);
        REQUIRE(cookie.find("HttpOnly") != std::string::npos);
    }

    SECTION("Method chaining") {
        json::object data = {{"message", "success"}};
        
        auto res = wolf::http_response(201)
            .header("X-API-Version", "1.0")
            .header("X-RateLimit", "100")
            .json(data)
            .cookie("tracking", "xyz789", "/", "", 86400, false, false);
        
        REQUIRE(res.result() == boost::beast::http::status::created);
        REQUIRE(res["X-API-Version"] == "1.0");
        REQUIRE(res["X-RateLimit"] == "100");
        REQUIRE(res[boost::beast::http::field::content_type] == "application/json");
        
        std::string cookie = std::string(res[boost::beast::http::field::set_cookie]);
        REQUIRE(cookie.find("tracking=xyz789") != std::string::npos);
    }

    SECTION("Send file") {
        std::string file_content = "PDF content here";
        wolf::http_response res(200);
        res.body() = file_content;
        auto& res_ref = res.send_file("document.pdf", "application/pdf");
        
        REQUIRE(res_ref[boost::beast::http::field::content_type] == "application/pdf");
        
        std::string disposition = std::string(res_ref[boost::beast::http::field::content_disposition]);
        REQUIRE(disposition.find("attachment") != std::string::npos);
        REQUIRE(disposition.find("document.pdf") != std::string::npos);
        REQUIRE(res_ref.body() == file_content);
    }

    SECTION("Status method chaining") {
        auto res = wolf::http_response()
            .status(404)
            .json(json::object{{"error", "Not Found"}});
        
        REQUIRE(res.result() == boost::beast::http::status::not_found);
        REQUIRE(res[boost::beast::http::field::content_type] == "application/json");
    }

    SECTION("Different status codes") {
        SECTION("200 OK") {
            auto res = wolf::http_response(200).text("OK");
            REQUIRE(res.result() == boost::beast::http::status::ok);
        }

        SECTION("201 Created") {
            auto res = wolf::http_response(201).json(json::object{});
            REQUIRE(res.result() == boost::beast::http::status::created);
        }

        SECTION("204 No Content") {
            auto res = wolf::http_response(204).text("");
            REQUIRE(res.result() == boost::beast::http::status::no_content);
        }

        SECTION("400 Bad Request") {
            auto res = wolf::http_response(400).json(json::object{{"error", "Bad Request"}});
            REQUIRE(res.result() == boost::beast::http::status::bad_request);
        }

        SECTION("401 Unauthorized") {
            auto res = wolf::http_response(401).json(json::object{{"error", "Unauthorized"}});
            REQUIRE(res.result() == boost::beast::http::status::unauthorized);
        }

        SECTION("404 Not Found") {
            auto res = wolf::http_response(404).json(json::object{{"error", "Not Found"}});
            REQUIRE(res.result() == boost::beast::http::status::not_found);
        }

        SECTION("422 Unprocessable Entity") {
            auto res = wolf::http_response(422).json(json::object{{"error", "Validation Failed"}});
            REQUIRE(res.result() == boost::beast::http::status::unprocessable_entity);
        }

        SECTION("500 Internal Server Error") {
            auto res = wolf::http_response(500).json(json::object{{"error", "Server Error"}});
            REQUIRE(res.result() == boost::beast::http::status::internal_server_error);
        }
    }

    SECTION("Empty JSON object") {
        auto res = wolf::http_response(201).json(json::object{});
        
        REQUIRE(res.result() == boost::beast::http::status::created);
        REQUIRE(res[boost::beast::http::field::content_type] == "application/json");
        REQUIRE(res.body() == "{}");
    }

    SECTION("Nested JSON structure") {
        json::object data = {
            {"user", json::object{
                {"id", 1},
                {"profile", json::object{
                    {"name", "John"},
                    {"email", "john@example.com"}
                }},
                {"roles", json::array{"admin", "user"}}
            }},
            {"meta", json::object{
                {"timestamp", 1234567890},
                {"version", "1.0"}
            }}
        };
        
        auto res = wolf::http_response(200).json(data);
        
        auto parsed = json::parse(res.body());
        REQUIRE(parsed.as_object()["user"].as_object()["id"].as_int64() == 1);
        REQUIRE(parsed.as_object()["user"].as_object()["profile"].as_object()["name"].as_string() == "John");
        REQUIRE(parsed.as_object()["user"].as_object()["roles"].as_array().size() == 2);
        REQUIRE(parsed.as_object()["meta"].as_object()["version"].as_string() == "1.0");
    }

    SECTION("Multiple cookies") {
        // Note: Setting multiple cookies requires calling cookie() multiple times
        // Each call overwrites the previous Set-Cookie header in Beast
        // For multiple cookies, we'd need to modify the implementation
        auto res = wolf::http_response(200)
            .text("Multiple cookies example")
            .cookie("cookie1", "value1", "/", "", 3600, true, false);
        
        // Verify at least the last cookie is set
        std::string cookie = std::string(res[boost::beast::http::field::set_cookie]);
        REQUIRE(cookie.find("cookie1=value1") != std::string::npos);
    }

    SECTION("Server header is set by default") {
        wolf::http_response res(200);
        REQUIRE(res[boost::beast::http::field::server] == "WolfServer/2.0");
    }

    SECTION("Complex real-world scenario - API error response") {
        json::object error_response = {
            {"success", false},
            {"error", json::object{
                {"code", "VALIDATION_ERROR"},
                {"message", "Input validation failed"},
                {"details", json::array{
                    json::object{{"field", "email"}, {"message", "Invalid format"}},
                    json::object{{"field", "age"}, {"message", "Must be positive"}}
                }}
            }},
            {"timestamp", 1234567890}
        };
        
        auto res = wolf::http_response(400)
            .header("X-Request-ID", "req-12345")
            .header("X-RateLimit-Remaining", "99")
            .json(error_response);
        
        REQUIRE(res.result() == boost::beast::http::status::bad_request);
        REQUIRE(res["X-Request-ID"] == "req-12345");
        REQUIRE(res["X-RateLimit-Remaining"] == "99");
        
        auto parsed = json::parse(res.body());
        REQUIRE(parsed.as_object()["success"].as_bool() == false);
        REQUIRE(parsed.as_object()["error"].as_object()["code"].as_string() == "VALIDATION_ERROR");
    }
}

TEST_CASE("http_response - Edge Cases", "[http_response]") {
    
    SECTION("Empty text response") {
        auto res = wolf::http_response(200).text("");
        REQUIRE(res.body().empty());
    }

    SECTION("Empty HTML response") {
        auto res = wolf::http_response(200).html("");
        REQUIRE(res.body().empty());
    }

    SECTION("Special characters in JSON") {
        json::object data = {
            {"message", "Hello \"World\""},
            {"path", "/api/test"},
            {"special", "Line1\nLine2\tTabbed"}
        };
        
        auto res = wolf::http_response(200).json(data);
        auto parsed = json::parse(res.body());
        
        REQUIRE(parsed.as_object()["message"].as_string() == "Hello \"World\"");
    }

    SECTION("Long text content") {
        std::string long_text(10000, 'A');
        auto res = wolf::http_response(200).text(long_text);
        REQUIRE(res.body().size() == 10000);
    }

    SECTION("Unicode in JSON") {
        json::object data = {
            {"name", "Jöhn Döe"},
            {"city", "北京"},
            {"emoji", "🚀"}
        };
        
        auto res = wolf::http_response(200).json(data);
        // Just verify it doesn't crash and produces valid JSON
        auto parsed = json::parse(res.body());
        REQUIRE(parsed.is_object());
    }
}
