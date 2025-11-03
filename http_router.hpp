#include <boost/unordered_map.hpp>
#include <boost/json.hpp>
#include <boost/tuple/tuple.hpp>
#include <memory>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <optional>

namespace wolf {
    enum http_method {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH,
        OPTIONS,
        HEAD,
        CONNECT,
        TRACE
    };

    std::string_view method_to_string(http_method method) {
        switch (method) {
            case GET: return "GET";
            case POST: return "POST";
            case PUT: return "PUT";
            case DELETE: return "DELETE";
            case PATCH: return "PATCH";
            case OPTIONS: return "OPTIONS";
            case HEAD: return "HEAD";
            case CONNECT: return "CONNECT";
            case TRACE: return "TRACE";
            default: return "UNKNOWN";
        }
    }

    using params_t = boost::unordered_map<std::string, std::string>;

    template <typename Res>
    class trie_router {
        private:
            struct trie_node {
                boost::unordered_map<std::string, std::unique_ptr<trie_node>> children;
                Res handler = nullptr;
                bool is_end = false;
                bool is_param = false;
                std::string param_name;
            };

            std::unique_ptr<trie_node> root;

            std::vector<std::string> split_route(const std::string& route) {
                std::vector<std::string> segments;
                std::stringstream ss(route);
                std::string segment;
                
                while (std::getline(ss, segment, '/')) {
                    if (!segment.empty()) {
                        segments.push_back(segment);
                    }
                }
                return segments;
            }

        public:
            trie_router() : root(std::make_unique<trie_node>()) {}

            void insert(const std::string& route, const Res& handler) {
                auto segments = split_route(route);
                trie_node* current = root.get();

                for (const auto& segment : segments) {
                    if (segment[0] == ':') {
                        // Parameter segment
                        std::string param_name = segment.substr(1); // Remove ':'
                        std::string key = ":param";
                        
                        if (current->children.find(key) == current->children.end()) {
                            current->children[key] = std::make_unique<trie_node>();
                            current->children[key]->is_param = true;
                            current->children[key]->param_name = param_name;
                        }
                        current = current->children[key].get();
                    } else {
                        // Regular segment
                        if (current->children.find(segment) == current->children.end()) {
                            current->children[segment] = std::make_unique<trie_node>();
                        }
                        current = current->children[segment].get();
                    }
                }

                current->is_end = true;
                current->handler = handler;
            }

            std::pair<Res, params_t> search(const std::string& route) {
                auto segments = split_route(route);
                trie_node* current = root.get();
                params_t params;

                for (const auto& segment : segments) {
                    bool found = false;

                    // First try exact match
                    if (current->children.find(segment) != current->children.end()) {
                        current = current->children[segment].get();
                        found = true;
                    } 
                    // Then try parameter match
                    else if (current->children.find(":param") != current->children.end()) {
                        current = current->children[":param"].get();
                        params[current->param_name] = segment;
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
            bool is_trie_route(const std::string& route) {
                auto sp_idx = route.find(':');

                return sp_idx != std::string::npos && sp_idx + 1 < route.size() && route[sp_idx - 1] == '/' && std::isalnum(route[sp_idx + 1]);
            }

            http_router& add(http_method method, const std::string& route, const std::function<RT(PT)>& handler) {
                if (is_trie_route(route)) {
                    trie_router_.insert(route, handler);
                    return *this;
                }
                std::string key = std::string(method_to_string(method)) + ":" + route;
                routes_[key] = handler;
                return *this;
            }

            http_router& get(const std::string& route, const std::function<RT(PT)>& handler) {
                return add(GET, route, handler);
            }

            http_router& post(const std::string& route, const std::function<RT(PT)>& handler) {
                return add(POST, route, handler);
            }

            http_router& put(const std::string& route, const std::function<RT(PT)>& handler) {
                return add(PUT, route, handler);
            }

            http_router& del(const std::string& route, const std::function<RT(PT)>& handler) {
                return add(DELETE, route, handler);
            }

            http_router& patch(const std::string& route, const std::function<RT(PT)>& handler) {
                return add(PATCH, route, handler);
            }

            http_router& options(const std::string& route, const std::function<RT(PT)>& handler) {
                return add(OPTIONS, route, handler);
            }
            
            http_router& head(const std::string& route, const std::function<RT(PT)>& handler) {
                return add(HEAD, route, handler);
            }

            http_router& connect(const std::string& route, const std::function<RT(PT)>& handler) {
                return add(CONNECT, route, handler);
            }

            http_router& trace(const std::string& route, const std::function<RT(PT)>& handler) {
                return add(TRACE, route, handler);
            }

            std::pair<std::function<RT(PT)>, params_t> resolve_trie(http_method /*method*/, const std::string& route) {
                auto [handler, uri_params] = trie_router_.search(route);

                if (handler) {
                    return {handler, uri_params};
                }

                return {nullptr, {}};
            }

            // Returns a tuple: (is_trie_route, handler, resolved_route_if_trie)
            boost::tuple<bool, std::function<RT(PT)>, params_t> resolve(http_method method, const std::string& route) noexcept {
                std::string key = std::string(method_to_string(method)) + ":" + route;
                auto it = routes_.find(key);
                if (it != routes_.end()) {
                    return {false, it->second, {}};
                } else {
                    auto [handler, uri_params] = resolve_trie(method, route);
                    if (handler) {
                        return {true, handler, uri_params};
                    }
                }

                return {false, nullptr, {}};
            }

            auto get_socket_handler() const {
                return socket_handler_;
            }

            auto& set_socket_handler(const std::function<std::string(const std::string&)>& handler) {
                socket_handler_ = handler;
                return *this;
            }
        
        private:
            boost::unordered_map<std::string, std::function<RT(PT)>> routes_;
            trie_router<std::function<RT(PT)>> trie_router_;
            std::function<std::string(const std::string&)> socket_handler_;
    };

}