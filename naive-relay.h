#ifndef CPP_MODULE_NAIVE_RELAY_NAIVE_RELAY_H
#define CPP_MODULE_NAIVE_RELAY_NAIVE_RELAY_H

#include "lazy-frame.h"
#include "connection.h"
#include "deps/libuv/include/uv.h"

namespace tchannel {

class NaiveRelay {
public:
    NaiveRelay(uv_loop_t *loop);
    ~NaiveRelay();

    void listen(int serverPort, const char *serverHost);
    void onNewConnection();
    void handleFrame(LazyFrame* lazyFrame);

    char hostPort[128];

private:
    uv_tcp_t *server;
    uv_loop_t *loop;
    std::vector<RelayConnection*> connections;

    void handleInitRequest(LazyFrame* frame);
    void handleInitResponse(LazyFrame* frame);

};

}

#endif /* CPP_MODULE_NAIVE_RELAY_NAIVE_RELAY_H */
