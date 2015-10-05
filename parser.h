#ifndef CPP_MODULE_NAIVE_RELAY_PARSER_H
#define CPP_MODULE_NAIVE_RELAY_PARSER_H

#include <vector>
#include "deps/libuv/include/uv.h"
#include "deps/buffer-reader/buffer-reader.h"

namespace tchannel {

class BufferSlice {
public:
    BufferSlice();
    BufferSlice(char* buf, size_t start, size_t end);

    char* buf;
    size_t start;
    size_t end;
    size_t length;
};

class FrameParser {
public:
    FrameParser();
    // ~FrameParser();

    void write(char* buf, size_t size);
    bool hasFrameBuffers();
    BufferSlice getFrameBuffer();

private:
    void addRemainder(char* buf, size_t start, size_t end);
    BufferSlice concatRemainder(char* buf, size_t size);
    void pushFrameBuffer(char* buf, size_t start, size_t end);
    void readFrameLength(char* buf, int offset, size_t length);

    std::vector<BufferSlice> remainder;
    std::vector<BufferSlice> frameBuffers;
    size_t remainderLength;
    uint16_t frameLength;
    Buffer::BufferReader reader;
};

}

#endif /* CPP_MODULE_NAIVE_RELAY_PARSER_H */
