#include <vector>
#include <cstdint>
#include "deps/buffer-reader/buffer-reader.h"

namespace tchannel {

class LazyFrame;

class LazyFramePool {
public:
    LazyFrame* acquire();
    void release(LazyFrame* frame);

private:
    std::vector<LazyFrame> frames;
    std::vector<LazyFrame*> availableFrames;
};

class LazyFrame {
public:
    LazyFrame();

    uint32_t readId();
    uint8_t readFrameType();
    void writeId(uint32_t newId);

private:
    char* frameBuffer;
    Buffer::BufferReader reader;
    uint32_t newId;

    uint32_t oldId;
    bool oldIdCached;

    uint8_t frameType;
    bool frameTypeCached;
};

}
