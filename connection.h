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
        uv_loop_t* loop, tchannel::NaiveRelay* relay, std::string direction
    );
    ~RelayConnection();

    void accept(uv_stream_t* server);
    void readStart();
    void onSocketRead(ssize_t nread, const uv_buf_t* buf);
    void handleInitRequest(LazyFrame* frame);
    void handleInitResponse(LazyFrame* frame);
    void initWriteComplete(int status);

    std::string direction;

private:
    struct write_req_t {
        uv_write_t req;
        uv_buf_t buf;
    };

    tchannel::FrameParser parser;
    tchannel::LazyFramePool framePool;

    uv_tcp_t* socket;
    write_req_t* initWriteReq;
    NaiveRelay* relay;
    uv_loop_t* loop;
    int idCounter;
    size_t initBufferLength;
    /* guid */
    /* outReq dictionary */
    /* initialized */
    /* frameQueue */

    void sendInitResponse(LazyFrame* frame);
    int allocateId();
};

}

#endif /* CPP_MODULE_NAIVE_RELAY_CONNECTION_H */
