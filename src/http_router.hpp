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

    using params_t = boost::unordered_map<std::string, std::string>;

    // Forward declare response_t from boost::beast
    namespace beast = boost::beast;
    namespace http = beast::http;
    using response_t = http::response<http::string_body>;

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
                Res handler = Res{};
                bool is_end = false;
                bool is_param = false;
                std::string param_name;
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
            void insert(T&& route, const Res& handler) {
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
                        std::string segment_str(segment);
                        if (current->children.find(segment_str) == current->children.end()) {
                            current->children[segment_str] = std::make_unique<trie_node>();
                        }
                        current = current->children[segment_str].get();
                    }
                }

                current->is_end = true;
                current->handler = handler;
            }

            // Use std::optional for cleaner return (C++20)
            [[nodiscard]] std::pair<Res, params_t> search(std::string_view route) const {
                auto segments = split_route(route);
                const trie_node* current = root.get();
                params_t params;

                for (const auto& segment : segments) {
                    bool found = false;

                    // Convert segment to string for map lookup
                    std::string segment_str(segment);
                    
                    // First try exact match
                    auto it = current->children.find(segment_str);
                    if (it != current->children.end()) {
                        current = it->second.get();
                        found = true;
                    } 
                    // Then try parameter match
                    else if (auto param_it = current->children.find(":param"); 
                             param_it != current->children.end()) {
                        current = param_it->second.get();
                        params[current->param_name] = std::string(segment);
                        found = true;
                    }

                    if (!found) {
                        return {nullptr, {}};
                    }
                }

                if (current->is_end) {
                    return {current->handler, params};
                }

                return {nullptr, {}};
            }
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

            // C++20 concepts for type safety
            template<RouteString T>
            http_router& add(http_method method, T&& route, const std::function<RT(PT)>& handler) {
                std::string_view route_view = route;
                if (is_trie_route(route_view)) {
                    trie_router_.insert(std::forward<T>(route), handler);
                    return *this;
                }
                // Use std::format (C++20) for cleaner string building
                auto key = std::format("{}:{}", method_to_string(method), route_view);
                routes_[key] = handler;
                return *this;
            }

            // Overload for string-returning handlers (C++20 concepts)
            template<RouteString T>
            http_router& add(http_method method, T&& route, const std::function<std::string(PT)>& handler) {
                return add(method, std::forward<T>(route), [handler](PT req) -> RT {
                    RT res;
                    res.result(boost::beast::http::status::ok);
                    res.set(boost::beast::http::field::content_type, "text/plain");
                    res.body() = handler(req);
                    res.prepare_payload();
                    return res;
                });
            }

            // Overload for sync handlers on async routers - wrap sync response in coroutine
            template<RouteString T>
            requires std::same_as<RT, net::awaitable<http_response>>
            http_router& add(http_method method, T&& route, const std::function<http_response(PT)>& handler) {
                return add(method, std::forward<T>(route), [handler](PT req) -> net::awaitable<http_response> {
                    co_return handler(req);
                });
            }

            // Overload for handlers returning types derived from response_t (e.g., http_response)
            template<RouteString T, ResponseLike R>
            requires (!std::same_as<R, RT>)
            http_router& add(http_method method, T&& route, const std::function<R(PT)>& handler) {
                return add(method, std::forward<T>(route), [handler](PT req) -> RT {
                    // Call handler which returns derived type, then convert to base type
                    return static_cast<RT>(handler(req));
                });
            }

            // Overload for async handlers returning net::awaitable<RT>
            template<RouteString T>
            http_router& add(http_method method, T&& route, const std::function<net::awaitable<RT>(PT)>& handler) {
                std::string_view route_view = route;
                if (is_trie_route(route_view)) {
                    trie_router_.insert(std::forward<T>(route), handler);
                    co_return *this;
                }
                auto key = std::format("{}:{}", method_to_string(method), route_view);
                routes_[key] = handler;
                co_return *this;
            }

            // Overload for async handlers returning net::awaitable<string>
            template<RouteString T>
            http_router& add(http_method method, T&& route, const std::function<net::awaitable<std::string>(PT)>& handler) {
                co_return add(method, std::forward<T>(route), [handler](PT req) -> net::awaitable<RT> {
                    RT res;
                    res.result(boost::beast::http::status::ok);
                    res.set(boost::beast::http::field::content_type, "text/plain");
                    res.body() = co_await handler(req);
                    res.prepare_payload();
                    co_return res;
                });
            }

            // Overload for async handlers returning net::awaitable<ResponseLike>
            template<RouteString T, ResponseLike R>
            requires (!std::same_as<R, RT>)
            http_router& add(http_method method, T&& route, const std::function<net::awaitable<R>(PT)>& handler) {
                co_return add(method, std::forward<T>(route), [handler](PT req) -> net::awaitable<RT> {
                    co_return static_cast<RT>(co_await handler(req));
                });
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

            [[nodiscard]] std::pair<std::function<RT(PT)>, params_t> resolve_trie(
                http_method /*method*/, std::string_view route) const {
                auto [handler, uri_params] = trie_router_.search(route);

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
            trie_router<std::function<RT(PT)>> trie_router_;
            std::function<std::string(const std::string&)> socket_handler_;
    };

} // namespace wolf