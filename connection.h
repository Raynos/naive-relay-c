#ifndef CPP_MODULE_NAIVE_RELAY_CONNECTION_H
#define CPP_MODULE_NAIVE_RELAY_CONNECTION_H

#include <unordered_map>
#include "parser.h"
#include "lazy-frame.h"
#include "deps/libuv/include/uv.h"

namespace tchannel {

class NaiveRelay;

enum class ConnectionDirection : int {
    OUTGOING = 10,
    INCOMING = 20
};

enum class ConnectionState : int {
    INITIAL = 10,
    CONNECTING = 20,
    INITIALIZING = 30,
    INITIALIZED = 40
};

class RelayConnection {
public:
    RelayConnection(
        uv_loop_t* loop, NaiveRelay* relay, ConnectionDirection direction
    );
    ~RelayConnection();

    void accept(uv_stream_t* server);
    void connect(struct sockaddr_in addr);
    void onConnect(int status);
    void readStart();
    void onSocketRead(ssize_t nread, const uv_buf_t* buf);
    void handleInitRequest(LazyFrame* frame);
    void handleInitResponse(LazyFrame* frame);
    void initWriteComplete(int status);
    int allocateId();
    void unsafeWriteBuffer(char* buf, size_t length);

    ConnectionDirection direction;
    ConnectionState connState;
    std::unordered_map<int, LazyFrame*> reqMap;


private:
    struct write_req_t {
        uv_write_t req;
        uv_buf_t buf;
    };

    tchannel::FrameParser parser;
    // tchannel::LazyFramePool framePool;

    uv_tcp_t* socket;
    write_req_t* initWriteReq;
    uv_connect_t* connectReq;
    NaiveRelay* relay;
    uv_loop_t* loop;
    int idCounter;
    size_t initBufferLength;

    /* guid */
    /* outReq dictionary */
    /* initialized */
    /* frameQueue */

    void sendInitResponse(LazyFrame* frame);
    void sendInitRequest();
};

}

#endif /* CPP_MODULE_NAIVE_RELAY_CONNECTION_H */
