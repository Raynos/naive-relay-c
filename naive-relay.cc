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

NaiveRelay::NaiveRelay(
    uv_loop_t *loop, std::vector<struct sockaddr_in> destinations,
    int serverPort, const char* serverHost
) {
    this->destinations = destinations;
    this->loop = loop;
    this->serverPort = serverPort;
    this->serverHost = serverHost;
    this->roundRobinIndex = 0;

    std::stringstream ss;
    ss << serverHost << ":" << serverPort;
    this->hostPort = ss.str();

    // TODO remove noob malloc
    this->server = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    this->server->data = (void*) this;

    for (struct sockaddr_in addr : this->destinations) {
        RelayConnection* conn = new RelayConnection(
            this->loop, this, ConnectionDirection::OUTGOING
        );
        conn->connect(addr);
        this->outConnections.push_back(conn);
    }
}

NaiveRelay::~NaiveRelay() {
    free(this->server);
}

void NaiveRelay::listen() {
    int r;

    r = uv_tcp_init(this->loop, this->server);
    assert(!r && "could not init the server");

    struct sockaddr_in serverAddress;

    r = uv_ip4_addr(this->serverHost, this->serverPort, &serverAddress);
    assert(!r && "could not allocate address");

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
            this->framePool.release(lazyFrame);
            // TODO release lazyFrame->frameBuffer
            break;

        case 0x02:
            conn->handleInitResponse(lazyFrame);
            this->framePool.release(lazyFrame);
            // TODO release lazyFrame->frameBuffer
            break;

        case 0x03:
            this->forwardCallRequest(lazyFrame);
            // TODO release lazyFrame
            break;

        case 0x04:
            this->forwardCallResponse(lazyFrame);
            // TODO release lazyFrame
            break;

        default:
            // TODO do a thing with lazyFrame
            std::cerr << lazyFrame->toString() << std::endl;
            break;
    }
}

void NaiveRelay::forwardCallRequest(LazyFrame* lazyFrame) {
    int inId = lazyFrame->readId();
    (void) inId;

    RelayConnection* destConn = this->chooseConn();
    int outId = destConn->allocateId();

    lazyFrame->writeId(outId);

    auto pair = std::pair<int, LazyFrame*>(outId, lazyFrame);
    destConn->reqMap.insert(pair);

    // std::cerr << "Forwarding Call Req from: " <<
    //     inId << " to: " << outId << std::endl;

    destConn->unsafeWriteBuffer(
        lazyFrame->frameBuffer, lazyFrame->frameBufferLength
    );
}

void NaiveRelay::forwardCallResponse(LazyFrame* lazyFrame) {
    int frameId = lazyFrame->readId();
    RelayConnection* srcConn = lazyFrame->conn;

    assert(srcConn->reqMap.find(frameId) != srcConn->reqMap.end() && "should contain id");

    LazyFrame* reqFrame = srcConn->reqMap.at(frameId);
    srcConn->reqMap.erase(frameId);

    lazyFrame->writeId(reqFrame->oldId);

    // std::cerr << "Forwarding Call Res from: " <<
    //     frameId << " to: " << reqFrame->oldId << std::endl;

    reqFrame->conn->unsafeWriteBuffer(
        lazyFrame->frameBuffer, lazyFrame->frameBufferLength
    );

    // TODO Free both lazyFrames
}

RelayConnection* NaiveRelay::chooseConn() {
    // RelayConnection* conn = this->outConnections.at(0);
    RelayConnection* conn = this->outConnections.at(this->roundRobinIndex);
    this->roundRobinIndex++;
    if (this->roundRobinIndex == this->outConnections.size()) {
        this->roundRobinIndex = 0;
    }

    return conn;
}

void NaiveRelay::onNewConnection() {
    tchannel::RelayConnection* conn;

    // TODO remove noob new connection
    // TODO delete on close.
    conn = new RelayConnection(this->loop, this, ConnectionDirection::INCOMING);
    this->inConnections.push_back(conn);

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
