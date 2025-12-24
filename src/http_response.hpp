#pragma once

#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/unordered_map.hpp>
#include <boost/url.hpp>
#include <ranges>

namespace beast = boost::beast;
namespace url = boost::urls;
namespace json = boost::json;
namespace http = beast::http;

namespace wolf {
    using response_t = beast::http::response<beast::http::string_body>;

    template<typename T>
    concept StatusType = std::same_as<T, beast::http::status>;
    
    // C++20 concepts for type safety
    template<typename T>
    concept StringLike = std::convertible_to<T, std::string_view>;

    class http_response : public response_t {
        public:
            // Default constructor
            http_response() : response_t(beast::http::status::ok, 11) {
                this->set(beast::http::field::server, "WolfServer/2.0");
            }

            // Constructor with status code (int or beast::http::status)
            explicit http_response(int status_code) 
                : response_t(static_cast<beast::http::status>(status_code), 11) {
                this->set(beast::http::field::server, "WolfServer/2.0");
            }

            explicit http_response(beast::http::status status) 
                : response_t(status, 11) {
                this->set(beast::http::field::server, "WolfServer/2.0");
            }

            // Allow base class constructors
            using response_t::response_t;

            // C++20 concepts for type safety
            template<StatusType S>
            http_response& set_status(S status) & {
                this->result(status);
                return *this;
            }

            template<StatusType S>
            http_response set_status(S status) && {
                this->result(status);
                return std::move(*this);
            }

            // Fluent API: Set JSON body with automatic content-type
            // Lvalue reference version - returns reference for chaining
            [[nodiscard]] http_response& json(const boost::json::value& json_value) & {
                this->set(beast::http::field::content_type, "application/json");
                this->body() = boost::json::serialize(json_value);
                this->prepare_payload();
                return *this;
            }

            // Rvalue reference version - returns by value to avoid dangling reference
            [[nodiscard]] http_response json(const boost::json::value& json_value) && {
                this->set(beast::http::field::content_type, "application/json");
                this->body() = boost::json::serialize(json_value);
                this->prepare_payload();
                return std::move(*this);
            }

            // Fluent API: Set JSON body from object (convertible to json::value)
            [[nodiscard]] http_response& json(const boost::json::object& json_obj) & {
                return json(boost::json::value(json_obj));
            }

            [[nodiscard]] http_response json(const boost::json::object& json_obj) && {
                return std::move(*this).json(boost::json::value(json_obj));
            }

            // Fluent API: Set JSON body from array
            [[nodiscard]] http_response& json(const boost::json::array& json_arr) & {
                return json(boost::json::value(json_arr));
            }

            [[nodiscard]] http_response json(const boost::json::array& json_arr) && {
                return std::move(*this).json(boost::json::value(json_arr));
            }

            // Fluent API: Set plain text body
            [[nodiscard]] http_response& text(std::string_view body_text) & {
                this->set(beast::http::field::content_type, "text/plain");
                this->body() = body_text;
                this->prepare_payload();
                return *this;
            }

            [[nodiscard]] http_response text(std::string_view body_text) && {
                this->set(beast::http::field::content_type, "text/plain");
                this->body() = body_text;
                this->prepare_payload();
                return std::move(*this);
            }

            // Fluent API: Set HTML body
            [[nodiscard]] http_response& html(std::string_view html_content) & {
                this->set(beast::http::field::content_type, "text/html");
                this->body() = html_content;
                this->prepare_payload();
                return *this;
            }

            [[nodiscard]] http_response html(std::string_view html_content) && {
                this->set(beast::http::field::content_type, "text/html");
                this->body() = html_content;
                this->prepare_payload();
                return std::move(*this);
            }

            // Fluent API: Set custom header
            template<StringLike K, StringLike V>
            [[nodiscard]] http_response& header(K&& key, V&& value) & {
                this->set(std::forward<K>(key), std::forward<V>(value));
                return *this;
            }

            template<StringLike K, StringLike V>
            [[nodiscard]] http_response header(K&& key, V&& value) && {
                this->set(std::forward<K>(key), std::forward<V>(value));
                return std::move(*this);
            }

            // Fluent API: Set cookie (convenience wrapper)
            template<StringLike K, StringLike V>
            [[nodiscard]] http_response& cookie(
                K&& key,
                V&& value,
                std::string_view path = "/",
                std::string_view domain = "",
                int max_age = -1,
                bool http_only = true,
                bool secure = false) &
            {
                set_cookie(*this, std::forward<K>(key), std::forward<V>(value), 
                          path, domain, max_age, http_only, secure);
                return *this;
            }

            template<StringLike K, StringLike V>
            [[nodiscard]] http_response cookie(
                K&& key,
                V&& value,
                std::string_view path = "/",
                std::string_view domain = "",
                int max_age = -1,
                bool http_only = true,
                bool secure = false) &&
            {
                set_cookie(*this, std::forward<K>(key), std::forward<V>(value), 
                          path, domain, max_age, http_only, secure);
                return std::move(*this);
            }

            // Fluent API: Send file (set content-disposition)
            [[nodiscard]] http_response& send_file(std::string_view filename, 
                                                   std::string_view content_type = "application/octet-stream") & {
                this->set(beast::http::field::content_type, content_type);
                this->set(beast::http::field::content_disposition, 
                         std::format("attachment; filename=\"{}\"", filename));
                this->prepare_payload();
                return *this;
            }

            [[nodiscard]] http_response send_file(std::string_view filename, 
                                                  std::string_view content_type = "application/octet-stream") && {
                this->set(beast::http::field::content_type, content_type);
                this->set(beast::http::field::content_disposition, 
                         std::format("attachment; filename=\"{}\"", filename));
                this->prepare_payload();
                return std::move(*this);
            }

            // Fluent API: Set status and return reference for chaining
            [[nodiscard]] http_response& status(int status_code) & {
                this->result(static_cast<beast::http::status>(status_code));
                return *this;
            }

            [[nodiscard]] http_response status(int status_code) && {
                this->result(static_cast<beast::http::status>(status_code));
                return std::move(*this);
            }

            [[nodiscard]] http_response& status(beast::http::status status_val) & {
                this->result(status_val);
                return *this;
            }

            [[nodiscard]] http_response status(beast::http::status status_val) && {
                this->result(status_val);
                return std::move(*this);
            }
    };
}

