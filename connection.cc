#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "connection.h"
#include "naive-relay.h"
#include "deps/libuv/include/uv.h"
#include "deps/buffer-reader/buffer-reader.h"

namespace tchannel {

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

class BufferSlice;

static size_t initFrameSize(std::string hostPort);
static void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
static void on_conn_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);
static void on_init_write_cb(uv_write_t* req, int status);
static int writeFrameHeader(
    char* buf, int offset, size_t bufferLength, int type, int id
);
static int writeInitBody(
    char* buf, int offset, size_t bufferLength, std::string hostPort
);

RelayConnection::RelayConnection(
    uv_loop_t *loop, tchannel::NaiveRelay *relay, std::string direction
) {
    this->loop = loop;
    this->direction = direction;
    this->relay = relay;

    this->idCounter = 0;
    this->parser = tchannel::FrameParser();
    this->framePool = tchannel::LazyFramePool();

    // TODO remove noob malloc
    this->socket = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    this->socket->data = (void*) this;

    this->initBufferLength = initFrameSize(this->relay->hostPort);
    // TODO remove noob malloc
    this->initWriteReq = (write_req_t*) malloc(sizeof(write_req_t));
    // TODO remove noob malloc
    this->initWriteReq->buf = uv_buf_init(
        (char*) malloc(this->initBufferLength), this->initBufferLength
    );
    this->initWriteReq->req.data = (void*) this;
}

RelayConnection::~RelayConnection() {
    free(this->socket);

    // TODO delete initWriteReq() ?
}

void RelayConnection::accept(uv_stream_t *server) {
    int r;

    r = uv_tcp_init(this->loop, this->socket);
    assert(!r && "could not init incoming socket");

    r = uv_accept(server, (uv_stream_t*) this->socket);
    if (r) {
        fprintf(stderr, "Could not accept socket %d\n", r);
        uv_close((uv_handle_t*) this->socket, nullptr);
    } else {
        fprintf(stderr, "Got incoming socket\n");
    }
}

void RelayConnection::readStart() {
    uv_read_start((uv_stream_t*) this->socket, alloc_cb, on_conn_read);

    /* TODO if this->direction === out then sendInitRequest() */
}

void RelayConnection::onSocketRead(ssize_t nread, const uv_buf_t *buf) {
    BufferSlice frameBuffer;
    LazyFrame* lazyFrame;

    if (nread == UV_EOF) {
        uv_close((uv_handle_t*) this->socket, nullptr);
        fprintf(stderr, "Got unexpected EOF on incoming socket\n");
    } else if (nread > 0) {
        this->parser.write(buf->base, (size_t) nread);

        while (this->parser.hasFrameBuffers()) {
            frameBuffer = this->parser.getFrameBuffer();

            // TODO release back to pool
            lazyFrame = this->framePool.acquire(
                frameBuffer.buf, frameBuffer.length, this
            );
            this->relay->handleFrame(this, lazyFrame);
        }
    } else {
        uv_close((uv_handle_t*) this->socket, nullptr);
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

void RelayConnection::handleInitRequest(LazyFrame* frame) {
    (void) frame;

    this->sendInitResponse(frame);

    // TODO self.flushPending();
}

void RelayConnection::handleInitResponse(LazyFrame* frame) {
    (void) frame;
}

void RelayConnection::sendInitResponse(LazyFrame* frame) {
    int offset = 0;
    offset = writeFrameHeader(
        this->initWriteReq->buf.base,
        offset,
        this->initBufferLength,
        0x02,
        frame->readId()
    );
    offset = writeInitBody(
        this->initWriteReq->buf.base,
        offset,
        this->initBufferLength,
        this->relay->hostPort
    );

    uv_write(
        (uv_write_t*) this->initWriteReq,
        (uv_stream_t*) this->socket,
        &(this->initWriteReq->buf),
        1,
        on_init_write_cb
    );
}

void RelayConnection::initWriteComplete(int status) {
    assert(!status && "failed to write init req/res");

    free(this->initWriteReq->buf.base);
    free(this->initWriteReq);
}

int RelayConnection::allocateId() {
    return this->idCounter++;
}

static int writeFrameHeader(
    char* buf, int offset, size_t bufferLength, int type, int id
) {
    Buffer::BufferReader reader;
    reader = Buffer::BufferReader(buf, bufferLength);

    reader.offset = offset;

    // size:2
    reader.WriteUint16BE((uint16_t) bufferLength);
    // type:1
    reader.WriteInt8((uint8_t) type);
    // reserved:1
    reader.Skip(1);
    // id:4
    reader.WriteUint32BE((uint32_t) id);
    // reserved:8
    reader.Skip(8);

    return reader.offset;
}

static int writeInitBody(
    char* buf, int offset, size_t bufferLength, std::string hostPort
) {
    Buffer::BufferReader reader;
    reader = Buffer::BufferReader(buf, bufferLength);

    reader.offset = offset;

    // version:2
    reader.WriteUint16BE((uint16_t) 2);
    // nh:2
    reader.WriteUint16BE((uint16_t) 2);

    std::string hostPortKey = std::string("host_port");
    // hostPort key length
    reader.WriteUint16BE((uint16_t) hostPortKey.size());
    // hostPort key value
    reader.Write(&hostPortKey[0], hostPortKey.size());

    // hostPort value length
    reader.WriteUint16BE((uint16_t) hostPort.size());
    // hostPort value value
    reader.Write(&hostPort[0], hostPort.size());

    std::string processNameKey = std::string("process_name");
    // processName key length
    reader.WriteUint16BE((uint16_t) processNameKey.size());
    // processName key value
    reader.Write(&processNameKey[0], processNameKey.size());

    char processTitle[512];
    uv_get_process_title(processTitle, sizeof(processTitle));
    size_t processTitleLength = strlen(processTitle);
    // processName value length
    reader.WriteUint16BE((uint16_t) processTitleLength);
    // processName value value
    reader.Write(processTitle, processTitleLength);

    return reader.offset;
}

static size_t initFrameSize(std::string hostPort) {
    size_t bufferLength = 0;

    char buffer[512];
    uv_get_process_title(buffer, sizeof(buffer));

    bufferLength += 16; // frameHeader:16
    bufferLength += 2; // version:2
    bufferLength += 2; // nh:2
    bufferLength += 2 + std::string("host_port").size(); // hostPortKey
    bufferLength += 2 + hostPort.size(); // hostPortValue
    bufferLength += 2 + std::string("process_name").size();
    bufferLength += 2 + strlen(buffer);

    return bufferLength;
}

static void alloc_cb(uv_handle_t* /*handle*/, size_t size, uv_buf_t*buf) {
    // TODO slab allocator
    buf->base = (char*) malloc(size);
    buf->len = size;
}

static void on_conn_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) {
    RelayConnection* conn;
    conn = (RelayConnection*) tcp->data;

    conn->onSocketRead(nread, buf);
}

static void on_init_write_cb(uv_write_t* req, int status) {
    RelayConnection* conn;
    conn = (RelayConnection*) req->data;

    conn->initWriteComplete(status);
}

}
