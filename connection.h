#ifndef CPP_MODULE_NAIVE_RELAY_CONNECTION_H
#define CPP_MODULE_NAIVE_RELAY_CONNECTION_H

#include "parser.h"
#include "lazy-frame.h"
#include "deps/libuv/include/uv.h"

namespace tchannel {

class NaiveRelay;

class RelayConnection {
public:
    RelayConnection(
        uv_loop_t* loop, tchannel::NaiveRelay* relay, const char* direction
    );
    ~RelayConnection();

    void accept(uv_stream_t* server);
    void readStart();
    void onSocketRead(ssize_t nread, const uv_buf_t* buf);

    char direction[16];

private:
    tchannel::FrameParser parser;
    tchannel::LazyFramePool framePool;

    uv_tcp_t* socket;
    NaiveRelay* relay;
    uv_loop_t* loop;
    /* idCounter */
    /* guid */
    /* outReq dictionary */
    /* initialized */
    /* frameQueue */
};

}

#endif /* CPP_MODULE_NAIVE_RELAY_CONNECTION_H */
