#ifndef CPP_MODULE_NAIVE_RELAY_LAZY_FRAME_H
#define CPP_MODULE_NAIVE_RELAY_LAZY_FRAME_H

#include <vector>
#include <cstdint>
#include <string>
#include "deps/buffer-reader/buffer-reader.h"

namespace tchannel {

class LazyFrame;
class RelayConnection;

class LazyFramePool {
public:
    LazyFramePool();

    LazyFrame* acquire(char* frameBuffer, size_t size, RelayConnection* conn);
    void release(LazyFrame* frame);

private:
    std::vector<LazyFrame> frames;
    std::vector<LazyFrame*> availableFrames;
};

class LazyFrame {
public:
    LazyFrame();

    void init();
    void init(char* frameBuffer, size_t size, RelayConnection* conn);
    void clear();
    std::string toString();

    uint32_t readId();
    uint8_t readFrameType();
    void writeId(uint32_t newId);

    RelayConnection* conn;

private:
    char* frameBuffer;
    size_t frameBufferLength;
    Buffer::BufferReader reader;
    uint32_t newId;

    uint32_t oldId;
    bool oldIdCached;

    uint8_t frameType;
    bool frameTypeCached;
};

}

#endif /* CPP_MODULE_NAIVE_RELAY_LAZY_FRAME_H */
