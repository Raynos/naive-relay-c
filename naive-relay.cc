#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "deps/libuv/include/uv.h"
#include "naive-relay.h"
#include "connection.h"

#define DEFAULT_BACKLOG 128

namespace tchannel {

static void on_connection_cb(uv_stream_t *server, int status);

NaiveRelay::NaiveRelay(uv_loop_t *loop) {
    this->loop = loop;
    strncpy(this->hostPort, "", 128);

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

    sprintf(this->hostPort, "%s:%d", serverHost, serverPort);

    r = uv_tcp_bind(this->server, (struct sockaddr*) &serverAddress, 0);
    assert(!r && "could not bind to server socket");

    r = uv_listen((uv_stream_t*) this->server, DEFAULT_BACKLOG, on_connection_cb);
    assert(!r && "could not listen to tcp server");
}

void NaiveRelay::onNewConnection() {
    tchannel::RelayConnection *conn;

    // TODO remove noob new connection
    conn = new tchannel::RelayConnection(this->loop, this, "in");

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
