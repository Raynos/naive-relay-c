#ifndef CPP_MODULE_NAIVE_RELAY_NAIVE_RELAY_H
#define CPP_MODULE_NAIVE_RELAY_NAIVE_RELAY_H

#include "lazy-frame.h"
#include "connection.h"
#include "deps/libuv/include/uv.h"
#include <string>

namespace tchannel {

class NaiveRelay {
public:
    NaiveRelay(
        uv_loop_t* loop,
        std::vector<struct sockaddr_in> destinations,
        int serverPort,
        const char* serverHost
    );
    ~NaiveRelay();

    void listen();
    void onNewConnection();
    void handleFrame(RelayConnection* conn, LazyFrame* lazyFrame);

    std::string hostPort;
    int serverPort;
    const char* serverHost;
    tchannel::LazyFramePool framePool;

private:
    uv_tcp_t *server;
    uv_loop_t *loop;
    std::vector<RelayConnection*> inConnections;
    std::vector<struct sockaddr_in> destinations;
    std::vector<RelayConnection*> outConnections;
    uint roundRobinIndex;

    void forwardCallRequest(LazyFrame* frame);
    void forwardCallResponse(LazyFrame* frame);
    RelayConnection* chooseConn();

};

}

#endif /* CPP_MODULE_NAIVE_RELAY_NAIVE_RELAY_H */
