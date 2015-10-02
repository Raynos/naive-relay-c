#include <assert.h>
#include "lazy-frame.h"

namespace tchannel {

const int ID_OFFSET = 4;
const int TYPE_OFFSET = 2;

LazyFrame::LazyFrame() {
    this->frameBuffer = nullptr;
    this->reader = Buffer::BufferReader();

    this->oldId = 0;
    this->oldIdCached = false;
    this->newId = 0;
    this->frameType = 0;
    this->frameTypeCached = false;
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
