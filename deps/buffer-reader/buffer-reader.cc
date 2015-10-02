#include <string.h>
#include "buffer-reader.h"

namespace Buffer {

BufferReader::BufferReader() {
    this->error = false;
    this->offset = (size_t)0;
    this->buffer = (char*)0;
    this->length = (size_t)0;
}

BufferReader::BufferReader(char* buffer, size_t length) {
    this->error = false;
    this->offset = 0;
    this->buffer = buffer;
    this->length = length;
}

bool BufferReader::Error() {
    return this->error;
}

uint8_t BufferReader::ReadUint8() {
    if (this->error || !this->CheckRead(1))
    {
        this->error = true;
        return 0;
    }

    uint8_t result = (uint8_t)buffer[offset];

    this->offset += 1;

    return result;
}

uint16_t BufferReader::ReadUint16BE() {
    if (this->error || !this->CheckRead(2))
    {
        this->error = true;
        return 0;
    }

    uint16_t result = (uint16_t)(
        ((unsigned char)buffer[this->offset] << 8) |
        ((unsigned char)buffer[this->offset + 1])
    );

    this->offset += 2;

    return result;
}

uint32_t BufferReader::ReadUint32BE() {
    if (this->error || !this->CheckRead(4)) {
        this->error = true;
        return 0;
    }

    uint32_t result = (uint32_t)(
        ((unsigned char)buffer[this->offset] << 24) |
        ((unsigned char)buffer[this->offset + 1] << 16) |
        ((unsigned char)buffer[this->offset + 2] << 8) |
        ((unsigned char)buffer[this->offset + 3])
    );

    this->offset += 4;

    return result;
}

void BufferReader::WriteUint32BE(uint32_t value) {
    if (this->error || !this->CheckRead(4)) {
        this->error = true;
        return 0;
    }

    memcpy(&buffer[this->offset], &value, 4);

    this->offset += 4;
}

void BufferReader::Skip(size_t count) {
    if (this->error || !this->CheckRead(count)) {
        this->error = true;
        return;
    }

    this->offset += count;
}

void BufferReader::ReadUint8Buffer(struct BufferResult &result) {
    if (this->error) {
        return;
    }

    size_t bufferLength = (size_t)this->ReadUint8();

    if (this->error || !this->CheckRead(bufferLength)) {
        this->error = true;
        return;
    }

    result.length = (size_t)bufferLength;

    if (bufferLength > 0) {
        result.buffer = (char*)&this->buffer[this->offset];
        this->offset += bufferLength;
    } else {
        result.buffer = NULL;
    }
}

void BufferReader::ReadUint16BERange(struct BufferRange &range) {
    if (this->error) {
        return;
    }

    size_t bufferLength = (size_t)this->ReadUint16BE();

    if (this->error || !CheckRead(bufferLength)) {
        this->error = true;
        return;
    }

    range.start = this->offset;
    range.end = this->offset + bufferLength;
    this->offset += bufferLength;
}

bool BufferReader::CheckRead(size_t read) {
    return this->offset + read <= this->length;
}

}
