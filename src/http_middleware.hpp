#pragma once

namespace wolf {
    template <typename Request, typename Response>
    class http_middleware {
    public:
        virtual ~http_middleware() = default;
        virtual void before_request(Request&) = 0;
        virtual void after_response(Request&, Response&) = 0;
    };
};