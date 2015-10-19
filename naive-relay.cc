#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include "deps/libuv/include/uv.h"
#include "naive-relay.h"

#define DEFAULT_BACKLOG 128

namespace tchannel {

static void on_connection_cb(uv_stream_t *server, int status);

NaiveRelay::NaiveRelay(uv_loop_t *loop, std::vector<std::string> destinations) {
    this->destinations = destinations;
    this->loop = loop;

    // TODO remove noob malloc
    this->server = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    this->server->data = (void*) this;
}

NaiveRelay::~NaiveRelay() {
    free(this->server);
}

void NaiveRelay::listen(int serverPort, const char *serverHost) {
    int r;

    r = uv_tcp_init(this->loop, this->server);
    assert(!r && "could not init the server");

    struct sockaddr_in serverAddress;

    r = uv_ip4_addr(serverHost, serverPort, &serverAddress);
    assert(!r && "could not allocate address");

    std::stringstream ss;
    ss << serverHost << ":" << serverPort;
    this->hostPort = ss.str();

    r = uv_tcp_bind(this->server, (struct sockaddr*) &serverAddress, 0);
    assert(!r && "could not bind to server socket");

    r = uv_listen((uv_stream_t*) this->server, DEFAULT_BACKLOG, on_connection_cb);
    assert(!r && "could not listen to tcp server");
}


void NaiveRelay::handleFrame(RelayConnection* conn, LazyFrame* lazyFrame) {
    int frameType = (int) lazyFrame->readFrameType();

    switch (frameType) {
        case 0x01:
            conn->handleInitRequest(lazyFrame);
            // TODO free lazyFrame
            break;

        case 0x02:
            conn->handleInitResponse(lazyFrame);
            // TODO free lazyFrame
            break;

        default:
            // TODO do a thing with lazyFrame
            std::cerr << lazyFrame->toString() << std::endl;
            break;
    }
}

void NaiveRelay::onNewConnection() {
    tchannel::RelayConnection* conn;

    // TODO remove noob new connection
    // TODO free() on close.
    conn = new RelayConnection(this->loop, this, "in");
    this->connections.push_back(conn);

    conn->accept((uv_stream_t*) this->server);
    conn->readStart();
}

static void on_connection_cb(uv_stream_t *server, int status) {
    assert(status >= 0 && "libuv incoming connection error");

    tchannel::NaiveRelay *relay;
    relay = (tchannel::NaiveRelay*) server->data;

    relay->onNewConnection();
}

}
