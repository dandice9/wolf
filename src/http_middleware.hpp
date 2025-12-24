#pragma once
#include <boost/beast.hpp>

namespace wolf {
    template <typename Request, typename Response>
    class http_middleware {
    public:
        virtual ~http_middleware() = default;
        virtual bool before_request(Request&) = 0; // return false to block the request
        virtual void after_response(Request&, Response&) = 0;
        virtual Response blocked_response(const Request& req) {
            Response res;
            res.result(boost::beast::http::status::forbidden);
            res.body() = "403 Forbidden by middleware";
            res.prepare_payload();
            return res;
        }
    };
};