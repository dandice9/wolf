#pragma once
#include <boost/beast.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace wolf {
    namespace net = boost::asio;

    template <typename Request, typename Response>
    class http_middleware {
    public:
        virtual ~http_middleware() = default;
        
        // Async before_request - return false to block the request
        virtual net::awaitable<bool> before_request(Request&) = 0;
        
        // Async after_response - modify response after handler
        virtual net::awaitable<void> after_response(Request&, Response&) = 0;
        
        virtual Response blocked_response(const Request& req) {
            Response res;
            res.result(boost::beast::http::status::forbidden);
            res.body() = "403 Forbidden by middleware";
            res.prepare_payload();
            return res;
        }
    };
};