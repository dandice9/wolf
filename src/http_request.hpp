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
    using params_t = boost::unordered_map<std::string, std::string>;
    using request_t = beast::http::request<beast::http::string_body>;

    class http_request : public request_t {
        params_t query_params_;
        params_t uri_params_;
        boost::json::object json_body_;
        public:
            http_request(const request_t& req,
                         const params_t& uri_params,
                         const decltype(url::parse_query(std::string()))& query_params)
                : request_t(req), uri_params_(uri_params)
            {
                if(query_params) {  
                    for(const auto v : *query_params) {
                        query_params_[std::string(v.key)] = std::string(v.value);
                    }
                }
                
                // Parse JSON body safely
                boost::system::error_code ec;
                auto parsed = json::parse(this->body(), ec);
                
                if(!ec && parsed.is_object()) {
                    json_body_ = parsed.as_object();
                } else {
                    // Handle parse error or non-object body
                    json_body_ = {};
                }
            }

            auto get_json_body() const {
                return json_body_;
            }

            auto params() const {
                return uri_params_;
            }

            auto query_params() const {
                return query_params_;
            }

            // C++20 ranges for cleaner cookie parsing
            [[nodiscard]] auto cookies() const {
                params_t cookies;
                
                if (auto it = this->find(http::field::cookie); it != this->end()) {
                    std::string_view cookie_str = it->value();
                    
                    // Use C++20 ranges to split and process cookies
                    auto cookie_pairs = cookie_str 
                        | std::views::split(';')
                        | std::views::transform([](auto&& rng) {
                            return std::string_view(&*rng.begin(), std::ranges::distance(rng));
                        });
                    
                    for (auto pair_view : cookie_pairs) {
                        if (auto eq_pos = pair_view.find('='); eq_pos != std::string_view::npos) {
                            auto key = pair_view.substr(0, eq_pos);
                            auto value = pair_view.substr(eq_pos + 1);
                            
                            // Trim whitespace using C++20 ranges
                            auto trim = [](std::string_view sv) -> std::string {
                                auto start = std::ranges::find_if_not(sv, ::isspace);
                                auto end = std::ranges::find_if_not(sv | std::views::reverse, ::isspace).base();
                                return std::string(start, end);
                            };
                            
                            cookies[trim(key)] = trim(value);
                        }
                    }
                }
                
                return cookies;
            }

            [[nodiscard]] auto headers() const {
                params_t headers;
                for (const auto& field : this->base()) {
                    headers[std::string(field.name_string())] = std::string(field.value());
                }
                return headers;
            }

            auto find_query_param(std::string_view key) const -> std::optional<std::string> {
                if (auto it = query_params_.find(std::string(key)); it != query_params_.end()) {
                    return it->second;
                }
                return std::nullopt;
            }

            auto find_uri_param(std::string_view key) const -> std::optional<std::string> {
                if (auto it = uri_params_.find(std::string(key)); it != uri_params_.end()) {
                    return it->second;
                }
                return std::nullopt;
            }

            auto find_header(std::string_view key) const -> std::optional<std::string> {
                if(auto it = this->find(key); it != this->end()) {
                    return std::string(it->value());
                }
                return std::nullopt;
            }

            auto find_cookie(std::string_view key) const -> std::optional<std::string> {
                auto cookies = this->cookies();
                if(auto it = cookies.find(std::string(key)); it != cookies.end()) {
                    return it->second;
                }
                return std::nullopt;
            }

            // C++20 std::optional for cleaner return semantics
            [[nodiscard]] std::optional<std::string> get(std::string_view key) const noexcept {
                if (auto it = uri_params_.find(std::string(key)); it != uri_params_.end()) {
                    return it->second;
                }
                if (auto it = query_params_.find(std::string(key)); it != query_params_.end()) {
                    return it->second;
                }
                if(auto it = this->cookies().find(std::string(key)); it != this->cookies().end()) {
                    return it->second;
                }
                if(auto it = this->find(key); it != this->end()) {
                    return std::string(it->value());
                }
                
                return std::nullopt;
            }

            // Convenience method that returns empty string if not found
            [[nodiscard]] std::string get_or(std::string_view key, std::string_view default_val = "") const noexcept {
                return get(key).value_or(std::string(default_val));
            }
    };
}