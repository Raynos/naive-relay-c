#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "connection.h"
#include "deps/libuv/include/uv.h"

namespace tchannel {

class BufferSlice;

static void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
static void on_conn_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);

RelayConnection::RelayConnection(
    uv_loop_t *loop, tchannel::NaiveRelay *relay, const char *direction
) {
    this->loop = loop;
    strncpy(this->direction, direction, 16);
    this->relay = relay;

    this->parser = new tchannel::FrameParser;
    this->socket = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    this->socket->data = (void*) this;
}

RelayConnection::~RelayConnection() {
    delete this->parser;
    free(this->socket);
}

void RelayConnection::accept(uv_stream_t *server) {
    int r;

    r = uv_tcp_init(this->loop, this->socket);
    assert(!r && "could not init incoming socket");

    r = uv_accept(server, (uv_stream_t*) this->socket);
    if (r) {
        fprintf(stderr, "Could not accept socket %d", r);
        uv_close((uv_handle_t*) this->socket, NULL);
    } else {
        fprintf(stderr, "Got incoming socket\n");
    }
}

void RelayConnection::readStart() {
    uv_read_start((uv_stream_t*) this->socket, alloc_cb, on_conn_read);

    /* if this->direction === out then sendInitRequest() */
}

void RelayConnection::onSocketRead(ssize_t nread, const uv_buf_t *buf) {
    BufferSlice frameBuffer;

    if (nread == UV_EOF) {
        uv_close((uv_handle_t*) this->socket, NULL);
        fprintf(stderr, "Got unexpected EOF on incoming socket\n");
    } else if (nread > 0) {
        this->parser->write(buf->base, (size_t) nread);

        while (this->parser->hasFrameBuffers()) {
            frameBuffer = this->parser->getFrameBuffer();
            (void) frameBuffer;
            // create lazy frame
            // this->relay->handleFrame()
        }
    } else {
        fprintf(
            stderr,
            "Got unexpected error on reading incoming socket %d\n",
            (int) nread
        );
    }

    if (buf->base) {
        free(buf->base);
    }
}

static void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    (void) handle;

    buf->base = (char*) malloc(size);
    buf->len = size;
}

static void on_conn_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {

    tchannel::RelayConnection *conn;
    conn = (tchannel::RelayConnection*) tcp->data;

    conn->onSocketRead(nread, buf);
}

}
