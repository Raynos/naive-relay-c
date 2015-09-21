#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "deps/libuv/include/uv.h"
#include "naive-relay.h"

#define DEFAULT_BACKLOG 128

namespace tchannel {

void on_new_connection(uv_stream_t *server, int status) {
    assert(status >= 0 && "libuv incoming connection error");

    tchannel::NaiveRelay *relay;
    relay = (tchannel::NaiveRelay*) server->data;

    relay->onNewConnection();
}

NaiveRelay::NaiveRelay(uv_loop_t *loop) {
    this->loop = loop;
    this->server = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    this->server->data = (void*) this;
    strncpy(this->hostPort, "", 128);
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

    r = uv_listen((uv_stream_t*) this->server, DEFAULT_BACKLOG, on_new_connection);
    assert(!r && "could not listen to tcp server");
}

void NaiveRelay::onNewConnection() {
    int r;

    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(this->loop, client);

    r = uv_accept((uv_stream_t*) this->server, (uv_stream_t*) client);
    if (r) {
        fprintf(stderr, "Could not accept socket %d", r);
        uv_close((uv_handle_t*) client, NULL);
        // free(client)
    } else {
        fprintf(stderr, "Got incoming socket\n");
    }
}

}
