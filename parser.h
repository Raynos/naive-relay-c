#include <vector>
#include "deps/libuv/include/uv.h"

namespace tchannel {

class BufferSlice {
public:
    BufferSlice();
    BufferSlice(char* buf, size_t length);

    char* buf;
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
    void addRemainder(char* buf, size_t size);
    BufferSlice concatRemainder(char* buf, size_t size);
    void pushFrameBuffer(char* buf, size_t size);
    void readFrameLength(char* buf, int offset);

    std::vector<BufferSlice> remainder;
    std::vector<BufferSlice> frameBuffers;
    size_t remainderLength;
    size_t frameLength;
};

}
