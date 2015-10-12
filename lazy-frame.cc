#include <assert.h>
#include "lazy-frame.h"

namespace tchannel {

const int ID_OFFSET = 4;
const int TYPE_OFFSET = 2;
const int FRAME_POOL_SIZE = 1000;

LazyFramePool::LazyFramePool() {
    this->frames = std::vector<LazyFrame>(FRAME_POOL_SIZE);
    this->availableFrames = std::vector<LazyFrame*>(FRAME_POOL_SIZE);

    for (int i = 0; i < FRAME_POOL_SIZE; i++) {
        this->frames[i].init();
        this->availableFrames[i] = &this->frames[i];
    }
}

LazyFrame* LazyFramePool::acquire(
    char* frameBuffer, size_t size, RelayConnection* conn
) {
    if (this->availableFrames.size() == 0) {
        assert("LazyFramePool is empty");
        return nullptr;
    }

    LazyFrame* ptr = this->availableFrames.back();
    this->availableFrames.pop_back();
    ptr->init(frameBuffer, size, conn);

    return ptr;
}

void LazyFramePool::release(LazyFrame* frame) {
    this->availableFrames.push_back(frame);
}

LazyFrame::LazyFrame() {
    this->init();
}

void LazyFrame::init() {
    this->frameBuffer = nullptr;
    this->reader = Buffer::BufferReader();
    this->conn = nullptr;

    this->frameBufferLength = 0;
    this->oldId = 0;
    this->oldIdCached = false;
    this->newId = 0;
    this->frameType = 0;
    this->frameTypeCached = false;
}

void LazyFrame::init(char* frameBuffer, size_t size, RelayConnection* conn) {
    this->init();

    this->frameBuffer = frameBuffer;
    this->frameBufferLength = size;
    this->reader = Buffer::BufferReader(frameBuffer, size);
    this->conn = conn;
}

std::string LazyFrame::toString() {
    std::string prefix = "LazyFrame: ";

    return prefix + std::string(this->frameBuffer, this->frameBufferLength);
}

uint32_t LazyFrame::readId() {
    if (this->oldIdCached) {
        return this->oldId;
    }

    this->reader.offset = ID_OFFSET;
    uint32_t oldId = this->reader.ReadUint32BE();
    if (this->reader.Error()) {
        assert("Out of bounds read for id");
        return 0;
    }

    this->oldId = oldId;
    this->oldIdCached = true;
    return this->oldId;
}

uint8_t LazyFrame::readFrameType() {
    if (this->frameTypeCached) {
        return this->frameType;
    }

    this->reader.offset = TYPE_OFFSET;
    uint8_t frameType = this->reader.ReadUint8();
    if (this->reader.Error()) {
        assert("Out of bounds read for frame type");
        return 0;
    }

    this->frameType = frameType;
    this->frameTypeCached = true;
    return this->frameType;
}

void LazyFrame::writeId(uint32_t newId) {
    this->reader.offset = ID_OFFSET;

    this->reader.WriteUint32BE(newId);
    if (this->reader.Error()) {
        assert("Out of bounds write for new id");
        return;
    }

    this->newId = newId;
}

}
