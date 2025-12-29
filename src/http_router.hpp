#pragma once

#include <boost/unordered_map.hpp>
#include <boost/json.hpp>
#include <boost/beast.hpp>
#include <boost/asio/awaitable.hpp>
#include <memory>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <optional>
#include <concepts>
#include <ranges>
#include <span>
#include <string_view>
#include <format>
#include <source_location>
#include "http_middleware.hpp"
#include "http_response.hpp"
#include "http_request.hpp"
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;

namespace wolf {
    // Forward declarations
    class http_response;
    
    namespace net = boost::asio;

    // C++20 enum class for type safety
    enum class http_method {
        GET,
        POST,
        PUT,
        DEL,
        PATCH,
        OPTIONS,
        HEAD,
        CONNECT,
        TRACE
    };

    // Constexpr for compile-time evaluation (C++20)
    [[nodiscard]] constexpr std::string_view method_to_string(http_method method) noexcept {
        switch (method) {
            case http_method::GET: return "GET";
            case http_method::POST: return "POST";
            case http_method::PUT: return "PUT";
            case http_method::DEL: return "DELETE";
            case http_method::PATCH: return "PATCH";
            case http_method::OPTIONS: return "OPTIONS";
            case http_method::HEAD: return "HEAD";
            case http_method::CONNECT: return "CONNECT";
            case http_method::TRACE: return "TRACE";
        }
        return "UNKNOWN";
    }

    // Type trait to detect awaitable types
    template<typename T>
    struct is_awaitable : std::false_type {};

    template<typename T>
    struct is_awaitable<net::awaitable<T>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_awaitable_v = is_awaitable<T>::value;

    // C++20 Concepts for type safety
    template<typename T>
    concept HandlerCallable = requires(T handler) {
        { handler } -> std::convertible_to<std::function<void()>>;
    };

    template<typename T>
    concept RouteString = std::convertible_to<T, std::string_view>;

    // Concept to check if type is derived from or same as response_t
    template<typename T>
    concept ResponseLike = std::derived_from<T, response_t> || std::same_as<T, response_t>;

    // Concept for awaitable handlers
    template<typename T>
    concept Awaitable = is_awaitable_v<T>;

    template <typename Res>
    class trie_router {
        private:
            struct trie_node {
                boost::unordered_map<std::string, std::unique_ptr<trie_node>> children;
                // Store handlers per HTTP method to distinguish between GET/POST/etc on same route
                boost::unordered_map<http_method, Res> handlers;
                bool is_param = false;
                std::string param_name;
                
                [[nodiscard]] bool has_handler(http_method method) const noexcept {
                    return handlers.find(method) != handlers.end();
                }
                
                [[nodiscard]] bool has_any_handler() const noexcept {
                    return !handlers.empty();
                }
            };

            std::unique_ptr<trie_node> root;

            // C++20 ranges-based split with std::string_view
            [[nodiscard]] std::vector<std::string> split_route(std::string_view route) const {
                std::vector<std::string> segments;
                
                // Use C++20 ranges for efficient splitting
                auto split_view = route 
                    | std::views::split('/')
                    | std::views::filter([](auto&& seg) { 
                        return !std::ranges::empty(seg); 
                    });
                
                for (auto&& segment : split_view) {
                    segments.emplace_back(segment.begin(), segment.end());
                }
                
                return segments;
            }

        public:
            trie_router() : root(std::make_unique<trie_node>()) {}

            // Use std::string_view for efficiency (C++20)
            template<RouteString T>
            void insert(http_method method, T&& route, const Res& handler) {
                auto segments = split_route(std::forward<T>(route));
                trie_node* current = root.get();

                for (const auto& segment : segments) {
                    if (!segment.empty() && segment[0] == ':') {
                        // Parameter segment
                        std::string param_name(segment.substr(1)); // Remove ':'
                        const std::string key = ":param";
                        
                        if (current->children.find(key) == current->children.end()) {
                            current->children[key] = std::make_unique<trie_node>();
                            current->children[key]->is_param = true;
                            current->children[key]->param_name = param_name;
                        }
                        current = current->children[key].get();
                    } else {
                        // Regular segment (convert string_view to string for map key)
                        if (current->children.find(segment) == current->children.end()) {
                            current->children[segment] = std::make_unique<trie_node>();
                        }
                        current = current->children[segment].get();
                    }
                }

                // Store handler for the specific HTTP method
                current->handlers[method] = handler;
            }

            // Use std::optional for cleaner return (C++20)
            [[nodiscard]] std::pair<Res, params_t> search(http_method method, std::string_view route) const {
                auto segments = split_route(route);
                const trie_node* current = root.get();
                params_t params;

                for (const auto& segment : segments) {
                    bool found = false;
                    
                    // First try exact match
                    auto it = current->children.find(segment);
                    if (it != current->children.end()) {
                        current = it->second.get();
                        found = true;
                    } 
                    // Then try parameter match
                    else {
                        auto param_it = current->children.find(":param");

                        if (param_it != current->children.end()) {
                            current = param_it->second.get();
                            params[current->param_name] = segment;
                            found = true;
                        }
                    }

                    if (!found) {
                        return {nullptr, {}};
                    }
                }

                // Look up handler for the specific HTTP method
                auto handler_it = current->handlers.find(method);
                if (handler_it != current->handlers.end()) {
                    return {handler_it->second, params};
                }

                return {nullptr, {}};
            }
    };

    // Unified handler wrapper that can hold both sync and async handlers
    template<typename PT>
    class unified_handler {
    public:
        using sync_fn = std::function<response_t(PT)>;
        using async_fn = std::function<net::awaitable<response_t>(PT)>;
        using handler_variant = std::variant<std::monostate, sync_fn, async_fn>;
        
        unified_handler() = default;
        
        // Constructor for sync handlers
        unified_handler(sync_fn fn) : handler_(std::move(fn)) {}
        
        // Constructor for async handlers
        unified_handler(async_fn fn) : handler_(std::move(fn)) {}
        
        [[nodiscard]] bool is_async() const noexcept { 
            return std::holds_alternative<async_fn>(handler_);
        }
        
        [[nodiscard]] net::awaitable<response_t> operator()(PT req) const {
            if (std::holds_alternative<async_fn>(handler_)) {
                co_return co_await std::get<async_fn>(handler_)(req);
            } else if (std::holds_alternative<sync_fn>(handler_)) {
                co_return std::get<sync_fn>(handler_)(req);
            } else {
                // Empty handler - return error response
                response_t res;
                res.result(boost::beast::http::status::internal_server_error);
                res.body() = "Handler not initialized";
                res.prepare_payload();
                co_return res;
            }
        }
        
        explicit operator bool() const noexcept {
            return !std::holds_alternative<std::monostate>(handler_);
        }
        
    private:
        handler_variant handler_;
    };

    using generic_middleware_t = http_middleware<http_request, http_response>;
    using middleware_list_t = std::vector<std::unique_ptr<generic_middleware_t>>;
    
    // Structure to hold pattern-based middleware
    struct pattern_middleware {
        std::string pattern;  // e.g., "/api/auth/*" or "/api/*"
        std::unique_ptr<generic_middleware_t> middleware;
        
        pattern_middleware(std::string p, std::unique_ptr<generic_middleware_t> m)
            : pattern(std::move(p)), middleware(std::move(m)) {}
    };

    template <typename RT, typename PT>
    class http_router {
        public:
            // C++20 constexpr and [[nodiscard]]
            [[nodiscard]] constexpr bool is_trie_route(std::string_view route) const noexcept {
                auto sp_idx = route.find(':');
                return sp_idx != std::string_view::npos && 
                       sp_idx + 1 < route.size() && 
                       route[sp_idx - 1] == '/' && 
                       std::isalnum(static_cast<unsigned char>(route[sp_idx + 1]));
            }
            
            // Check if a route matches a wildcard pattern
            // Patterns: "/api/*" matches "/api/users", "/api/auth/login", etc.
            [[nodiscard]] static bool matches_pattern(std::string_view pattern, std::string_view route) noexcept {
                // Check for wildcard at end
                if (pattern.ends_with("/*")) {
                    auto prefix = pattern.substr(0, pattern.size() - 1); // Remove "*", keep "/"
                    return route.starts_with(prefix) || route == pattern.substr(0, pattern.size() - 2);
                }
                // Check for wildcard anywhere (e.g., "/api/*/users")
                if (auto pos = pattern.find('*'); pos != std::string_view::npos) {
                    auto prefix = pattern.substr(0, pos);
                    auto suffix = pattern.substr(pos + 1);
                    
                    if (!route.starts_with(prefix)) return false;
                    if (suffix.empty()) return true;
                    
                    // Find suffix in remaining route
                    auto remaining = route.substr(prefix.size());
                    return remaining.find(suffix) != std::string_view::npos;
                }
                // Exact match
                return pattern == route;
            }

            // Unified add() for both sync and async handlers
            template<RouteString T, typename Handler>
            requires std::invocable<Handler, PT>
            http_router& add(http_method method, T&& route, Handler&& handler) {
                using return_t = std::invoke_result_t<Handler, PT>;
                constexpr bool is_async_handler = is_awaitable_v<return_t>;
                
                // Convert handler to the expected function signature
                std::function<RT(PT)> wrapped_handler;
                
                if constexpr (is_async_handler) {
                    // Async handler path
                    using inner_t = typename return_t::value_type;
                    
                    if constexpr (std::same_as<return_t, RT>) {
                        wrapped_handler = std::forward<Handler>(handler);
                    }
                    else if constexpr (std::same_as<inner_t, std::string>) {
                        wrapped_handler = [h = std::forward<Handler>(handler)](PT req) -> RT {
                            typename RT::value_type res;
                            res.result(boost::beast::http::status::ok);
                            res.set(boost::beast::http::field::content_type, "text/plain");
                            res.body() = co_await h(req);
                            res.prepare_payload();
                            co_return res;
                        };
                    }
                    else if constexpr (ResponseLike<inner_t>) {
                        wrapped_handler = [h = std::forward<Handler>(handler)](PT req) -> RT {
                            co_return static_cast<typename RT::value_type>(co_await h(req));
                        };
                    }
                } else {
                    // Sync handler path
                    if constexpr (std::same_as<return_t, RT>) {
                        wrapped_handler = std::forward<Handler>(handler);
                    }
                    else if constexpr (std::same_as<return_t, std::string>) {
                        if constexpr (is_awaitable_v<RT>) {
                            wrapped_handler = [h = std::forward<Handler>(handler)](PT req) -> RT {
                                typename RT::value_type res;
                                res.result(boost::beast::http::status::ok);
                                res.set(boost::beast::http::field::content_type, "text/plain");
                                res.body() = h(req);
                                res.prepare_payload();
                                co_return res;
                            };
                        } else {
                            wrapped_handler = [h = std::forward<Handler>(handler)](PT req) -> RT {
                                RT res;
                                res.result(boost::beast::http::status::ok);
                                res.set(boost::beast::http::field::content_type, "text/plain");
                                res.body() = h(req);
                                res.prepare_payload();
                                return res;
                            };
                        }
                    }
                    else if constexpr (ResponseLike<return_t>) {
                        if constexpr (is_awaitable_v<RT>) {
                            wrapped_handler = [h = std::forward<Handler>(handler)](PT req) -> RT {
                                auto response = h(req);
                                if constexpr (std::same_as<typename RT::value_type, return_t>) {
                                    co_return response;
                                } else {
                                    typename RT::value_type result;
                                    result.result(response.result());
                                    result.body() = std::move(response.body());
                                    for (auto const& field : response) {
                                        result.set(field.name(), field.value());
                                    }
                                    result.version(response.version());
                                    result.prepare_payload();
                                    co_return result;
                                }
                            };
                        } else {
                            wrapped_handler = [h = std::forward<Handler>(handler)](PT req) -> RT {
                                return static_cast<RT>(h(req));
                            };
                        }
                    }
                }
                
                // Insert into router
                std::string_view route_view = route;
                if (is_trie_route(route_view)) {
                    trie_router_.insert(method, std::forward<T>(route), wrapped_handler);
                } else {
                    auto key = std::format("{}:{}", method_to_string(method), route_view);
                    routes_[key] = wrapped_handler;
                }

                this->last_added_path_ = route_view;

                return *this;
            }

            // Generic callable overloads that accept lambdas
            template<RouteString T, typename Callable>
            requires std::invocable<Callable, PT>
            http_router& get(T&& route, Callable&& handler) {
                using result_t = std::invoke_result_t<Callable, PT>;
                return add(http_method::GET, std::forward<T>(route), 
                          std::function<result_t(PT)>(std::forward<Callable>(handler)));
            }

            // Generic callable overload for POST
            template<RouteString T, typename Callable>
            requires std::invocable<Callable, PT>
            http_router& post(T&& route, Callable&& handler) {
                using result_t = std::invoke_result_t<Callable, PT>;
                return add(http_method::POST, std::forward<T>(route), 
                          std::function<result_t(PT)>(std::forward<Callable>(handler)));
            }

            // Generic callable overload for PUT
            template<RouteString T, typename Callable>
            requires std::invocable<Callable, PT>
            http_router& put(T&& route, Callable&& handler) {
                using result_t = std::invoke_result_t<Callable, PT>;
                return add(http_method::PUT, std::forward<T>(route), 
                          std::function<result_t(PT)>(std::forward<Callable>(handler)));
            }

            // Generic callable overload for DEL
            template<RouteString T, typename Callable>
            requires std::invocable<Callable, PT>
            http_router& del(T&& route, Callable&& handler) {
                using result_t = std::invoke_result_t<Callable, PT>;
                return add(http_method::DEL, std::forward<T>(route), 
                          std::function<result_t(PT)>(std::forward<Callable>(handler)));
            }

            // Generic callable overload for PATCH
            template<RouteString T, typename Callable>
            requires std::invocable<Callable, PT>
            http_router& patch(T&& route, Callable&& handler) {
                using result_t = std::invoke_result_t<Callable, PT>;
                return add(http_method::PATCH, std::forward<T>(route), 
                          std::function<result_t(PT)>(std::forward<Callable>(handler)));
            }

            // Generic callable overload for OPTIONS
            template<RouteString T, typename Callable>
            requires std::invocable<Callable, PT>
            http_router& options(T&& route, Callable&& handler) {
                using result_t = std::invoke_result_t<Callable, PT>;
                return add(http_method::OPTIONS, std::forward<T>(route), 
                          std::function<result_t(PT)>(std::forward<Callable>(handler)));
            }
            
            // Generic callable overload for HEAD
            template<RouteString T, typename Callable>
            requires std::invocable<Callable, PT>
            http_router& head(T&& route, Callable&& handler) {
                using result_t = std::invoke_result_t<Callable, PT>;
                return add(http_method::HEAD, std::forward<T>(route), 
                          std::function<result_t(PT)>(std::forward<Callable>(handler)));
            }

            // Generic callable overload for CONNECT
            template<RouteString T, typename Callable>
            requires std::invocable<Callable, PT>
            http_router& connect(T&& route, Callable&& handler) {
                using result_t = std::invoke_result_t<Callable, PT>;
                return add(http_method::CONNECT, std::forward<T>(route), 
                          std::function<result_t(PT)>(std::forward<Callable>(handler)));
            }

            // Generic callable overload for TRACE
            template<RouteString T, typename Callable>
            requires std::invocable<Callable, PT>
            http_router& trace(T&& route, Callable&& handler) {
                using result_t = std::invoke_result_t<Callable, PT>;
                return add(http_method::TRACE, std::forward<T>(route), 
                          std::function<result_t(PT)>(std::forward<Callable>(handler)));
            }

            // Attach middleware to a specific route (last added route)
            http_router& attach(std::unique_ptr<generic_middleware_t> middleware) {
                if(!this->last_added_path_.empty())
                {
                    if(middlewares_.find(std::string(this->last_added_path_)) == middlewares_.end())
                        middlewares_[std::string(this->last_added_path_)] = middleware_list_t{};

                    middlewares_[std::string(this->last_added_path_)].emplace_back(std::move(middleware));
                }
                return *this;
            }
            
            // Attach middleware using a wildcard pattern
            // Examples: "/api/*", "/api/auth/*", "/*" (all routes)
            template<RouteString T>
            http_router& use(T&& pattern, std::unique_ptr<generic_middleware_t> middleware) {
                pattern_middlewares_.emplace_back(
                    std::string(std::forward<T>(pattern)), 
                    std::move(middleware)
                );
                return *this;
            }
            
            // Attach middleware to all routes (shorthand for "/*")
            http_router& use(std::unique_ptr<generic_middleware_t> middleware) {
                return use("/*", std::move(middleware));
            }

            // Get all middlewares for a specific route (exact + pattern matches)
            // Returns pointers to middlewares (non-owning), caller should not delete
            [[nodiscard]] std::vector<generic_middleware_t*> get_middlewares(std::string_view route) const {
                std::vector<generic_middleware_t*> result;
                
                // First, collect pattern-based middlewares (in order they were registered)
                for (const auto& pm : pattern_middlewares_) {
                    if (matches_pattern(pm.pattern, route)) {
                        result.push_back(pm.middleware.get());
                    }
                }
                
                // Then, collect route-specific middlewares
                auto it = middlewares_.find(std::string(route));
                if (it != middlewares_.end()) {
                    for (const auto& m : it->second) {
                        result.push_back(m.get());
                    }
                }
                
                return result;
            }

            [[nodiscard]] std::pair<std::function<RT(PT)>, params_t> resolve_trie(
                http_method method, std::string_view route) const {
                auto [handler, uri_params] = trie_router_.search(method, route);

                if (handler) {
                    return {handler, uri_params};
                }

                return {nullptr, {}};
            }

            // Returns a tuple: (is_trie_route, handler, resolved_route_if_trie)
            // Using structured bindings (C++20)
            [[nodiscard]] std::tuple<bool, std::function<RT(PT)>, params_t> resolve(
                http_method method, std::string_view route) const noexcept {
                // Use std::format for key construction (C++20)
                auto key = std::format("{}:{}", method_to_string(method), route);
                
                if (auto it = routes_.find(key); it != routes_.end()) {
                    return {false, it->second, {}};
                }
                
                auto [handler, uri_params] = resolve_trie(method, route);

                if (handler) {
                    return {true, handler, uri_params};
                }

                return {false, nullptr, {}};
            }

            [[nodiscard]] auto get_socket_handler() const noexcept {
                return socket_handler_;
            }

            auto& set_socket_handler(const std::function<std::string(const std::string&)>& handler) noexcept {
                socket_handler_ = handler;
                return *this;
            }
        
        private:
            boost::unordered_map<std::string, std::function<RT(PT)>> routes_;
            boost::unordered_map<std::string, middleware_list_t> middlewares_;
            std::vector<pattern_middleware> pattern_middlewares_;  // Wildcard pattern middlewares
            trie_router<std::function<RT(PT)>> trie_router_;
            std::function<std::string(const std::string&)> socket_handler_;
            std::string_view last_added_path_ = "";
    };

} // namespace wolf