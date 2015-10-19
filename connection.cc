#include <iostream>
#include <cstring>
#include <cassert>
#include "connection.h"
#include "naive-relay.h"
#include "deps/libuv/include/uv.h"
#include "deps/buffer-reader/buffer-reader.h"

namespace tchannel {

class BufferSlice;

static size_t initFrameSize(std::string hostPort);
static void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
static void on_conn_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);
static void on_connect_cb(uv_connect_t* req, int status);
static void on_init_write_cb(uv_write_t* req, int status);
static void on_unsafe_write_cb(uv_write_t* req, int status);
static int writeFrameHeader(
    char* buf, int offset, size_t bufferLength, int type, int id
);
static int writeInitBody(
    char* buf, int offset, size_t bufferLength, std::string hostPort
);

RelayConnection::RelayConnection(
    uv_loop_t *loop, NaiveRelay *relay, ConnectionDirection direction
) {
    this->loop = loop;
    this->direction = direction;
    this->relay = relay;
    this->connState = ConnectionState::INITIAL;
    this->reqMap = std::unordered_map<int, LazyFrame*>();

    this->idCounter = 0;
    this->parser = tchannel::FrameParser();

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

    this->connectReq = (uv_connect_t*) malloc(sizeof(uv_connect_t));
    this->connectReq->data = (void*) this;
}

RelayConnection::~RelayConnection() {
    free(this->socket);

    // TODO delete initWriteReq() ?
    // TODO delete connectReq() ?
}

void RelayConnection::accept(uv_stream_t *server) {
    int r;

    r = uv_tcp_init(this->loop, this->socket);
    assert(!r && "could not init incoming socket");

    r = uv_accept(server, (uv_stream_t*) this->socket);
    if (r) {
        std::cerr << "Could not accept socket " << r << std::endl;
        uv_close((uv_handle_t*) this->socket, nullptr);
    } else {
        std::cerr << "Got incoming socket" << std::endl;
        this->connState = ConnectionState::INITIALIZING;
    }
}

void RelayConnection::connect(struct sockaddr_in addr) {
    int r;

    r = uv_tcp_init(this->loop, this->socket);
    assert(!r && "could not init outgoing socket");

    this->connState = ConnectionState::CONNECTING;
    r = uv_tcp_connect(
        this->connectReq, this->socket, (const struct sockaddr*) &addr, on_connect_cb
    );
    assert(!r && "Could not connect to addr");
}

void RelayConnection::onConnect(int status) {
    assert(!status && "failed to connect to destination");

    this->connState = ConnectionState::INITIALIZING;
    free(this->connectReq);

    this->readStart();
}

void RelayConnection::readStart() {
    uv_read_start((uv_stream_t*) this->socket, alloc_cb, on_conn_read);

    if (this->direction == ConnectionDirection::OUTGOING) {
        this->sendInitRequest();
    }
}

void RelayConnection::onSocketRead(ssize_t nread, const uv_buf_t *buf) {
    BufferSlice frameBuffer;
    LazyFrame* lazyFrame;

    if (nread == UV_EOF) {
        uv_close((uv_handle_t*) this->socket, nullptr);
        std::cerr << "Got unexpected EOF on " << (int) this->direction <<
            " socket" << std::endl;
    } else if (nread > 0) {
        this->parser.write(buf->base, (size_t) nread);

        while (this->parser.hasFrameBuffers()) {
            frameBuffer = this->parser.getFrameBuffer();

            // TODO release back to pool
            lazyFrame = this->relay->framePool.acquire(
                frameBuffer.buf, frameBuffer.length, this
            );
            this->relay->handleFrame(this, lazyFrame);
        }
    } else {
        uv_close((uv_handle_t*) this->socket, nullptr);
        std::cerr << "Got unexpected error on reading incoming socket "
            << nread << std::endl;
    }

    if (buf->base) {
        free(buf->base);
    }
}

void RelayConnection::handleInitRequest(LazyFrame* frame) {
    this->sendInitResponse(frame);

    // TODO self.flushPending();
    this->connState = ConnectionState::INITIALIZED;
}

void RelayConnection::handleInitResponse(LazyFrame* frame) {
    (void) frame;

    // TODO self.flushPending();
    this->connState = ConnectionState::INITIALIZING;
}

// TODO luuuuuuuuuuuuul unsafe
void RelayConnection::unsafeWriteBuffer(char* buffer, size_t length) {
    uv_write_t* writeReq = (uv_write_t*) malloc(sizeof(uv_write_t));
    uv_buf_t buf = uv_buf_init(buffer, length);

    std::cerr << "unsafeWriteBuffer: " <<
        std::string(buffer, length) << std::endl;

    uv_write(
        writeReq, (uv_stream_t*) this->socket, &buf, 1, on_unsafe_write_cb
    );
}

void RelayConnection::sendInitRequest() {
    int offset = 0;
    offset = writeFrameHeader(
        this->initWriteReq->buf.base,
        offset,
        this->initBufferLength,
        0x01,
        this->allocateId()
    );
    offset = writeInitBody(
        this->initWriteReq->buf.base,
        offset,
        this->initBufferLength,
        this->relay->hostPort
    );

    // std::cerr << "Writing InitRequest len: " <<
    //     this->initBufferLength << " InitRequest body: " <<
    //     std::string(this->initWriteReq->buf.base, this->initBufferLength) <<
    //     std::endl;

    uv_write(
        (uv_write_t*) this->initWriteReq,
        (uv_stream_t*) this->socket,
        &(this->initWriteReq->buf),
        1,
        on_init_write_cb
    );
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

    // std::cerr << "Writing InitResponse: " <<
    // this->initBufferLength << " InitResponse body: " <<
    //     std::string(this->initWriteReq->buf.base, this->initBufferLength) <<
    //     std::endl;

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

    size_t processTitleLength = std::strlen(processTitle);

    // std::cerr << "Got process title: " <<
    //     std::string(processTitle, processTitleLength) << std::endl;

    // processName value length
    reader.WriteUint16BE((uint16_t) processTitleLength);
    // processName value value
    reader.Write(processTitle, processTitleLength);

    return reader.offset;
}

static size_t initFrameSize(std::string hostPort) {
    size_t bufferLength = 0;

    char processTitle[512];
    uv_get_process_title(processTitle, sizeof(processTitle));

    // std::cerr << "Got process title: " <<
    //     std::string(processTitle, std::strlen(processTitle)) << std::endl;

    bufferLength += 16; // frameHeader:16
    bufferLength += 2; // version:2
    bufferLength += 2; // nh:2
    bufferLength += 2 + std::string("host_port").size(); // hostPortKey
    bufferLength += 2 + hostPort.size(); // hostPortValue
    bufferLength += 2 + std::string("process_name").size();
    bufferLength += 2 + std::strlen(processTitle);

    return bufferLength;
}

static void alloc_cb(uv_handle_t* /*handle*/, size_t size, uv_buf_t*buf) {
    // TODO slab allocator
    buf->base = (char*) malloc(size);
    buf->len = size;
}

static void on_conn_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) {
    RelayConnection* conn = (RelayConnection*) tcp->data;

    conn->onSocketRead(nread, buf);
}

static void on_init_write_cb(uv_write_t* req, int status) {
    RelayConnection* conn = (RelayConnection*) req->data;

    conn->initWriteComplete(status);
}

static void on_connect_cb(uv_connect_t* req, int status) {
    RelayConnection* conn = (RelayConnection*) req->data;

    conn->onConnect(status);
}

static void on_unsafe_write_cb(uv_write_t* req, int status) {
    (void) req;

    std::cerr << "Failed to write: " << uv_strerror(status) << std::endl;

    assert(!status && "unsafe write failed");
}

}
