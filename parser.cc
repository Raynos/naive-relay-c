#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "deps/libuv/include/uv.h"

namespace tchannel {

uint16_t readFrameSize(char* buf, int offset);
char* bufCopySlice(char* buf, int start, int end);

BufferSlice::BufferSlice() {
    this->buf = nullptr;
    this->length = (size_t) 0;
}

BufferSlice::BufferSlice(char* buf, size_t length) {
    this->buf = buf;
    this->length = length;
}

FrameParser::FrameParser() {
    this->remainderLength = 0;
    this->frameLength = 0;
}

// buf will be freed by caller.
void FrameParser::write(char* buf, size_t size) {
    if (this->frameLength == 0) {
        this->readFrameLength(buf, 0);
    }

    size_t totalBufferLength = size;
    size_t totalLength = this->remainderLength + totalBufferLength;

    if (this->frameLength == totalLength) {
        // TODO free `lastBuffer`
        char* lastBuffer = bufCopySlice(buf, 0, size);
        this->pushFrameBuffer(lastBuffer, size);
        return;
    }

    if (this->frameLength > totalLength) {
        // TODO free `remainderBuffer`
        char* remainderBuffer = bufCopySlice(buf, 0, size);
        this->addRemainder(remainderBuffer, size);
        return;
    }

    size_t startOfBuffer = 0;

    while (this->frameLength <= totalLength) {
        size_t amountToRead = this->frameLength - this->remainderLength;

        // TODO free `lastBuffer`
        char* lastBuffer = bufCopySlice(
            buf, startOfBuffer, startOfBuffer + amountToRead
        );
        this->pushFrameBuffer(lastBuffer, amountToRead);

        if (startOfBuffer + amountToRead == totalBufferLength) {
            return;
        }

        startOfBuffer = startOfBuffer + amountToRead;
        totalLength = totalBufferLength - startOfBuffer;
        this->readFrameLength(buf, startOfBuffer);
    }

    if (startOfBuffer < totalBufferLength) {
        char* remainderBuffer = bufCopySlice(
            buf, startOfBuffer, totalBufferLength
        );
        size_t size = totalBufferLength - startOfBuffer;
        this->addRemainder(remainderBuffer, size);
    }
}

bool FrameParser::hasFrameBuffers() {
    return this->frameBuffers.size() != 0;
}

BufferSlice FrameParser::getFrameBuffer() {
    assert(this->frameBuffers.size() > 0 && "frameBuffers is empty");

    BufferSlice last = this->frameBuffers.back();
    this->frameBuffers.pop_back();
    return last;
}

void FrameParser::pushFrameBuffer(char* buf, size_t size) {
    BufferSlice frameBuffer = this->concatRemainder(buf, size);
    this->frameBuffers.push_back(frameBuffer);
    this->frameLength = 0;
}

void FrameParser::addRemainder(char* buf, size_t size) {
    BufferSlice slice = BufferSlice(buf, size);
    this->remainder.push_back(slice);
    this->remainderLength += size;
}

BufferSlice FrameParser::concatRemainder(char* buf, size_t size) {
    BufferSlice slice;

    if (this->remainderLength == 0) {
        slice = BufferSlice(buf, size);
        return slice;
    }

    // TODO bug.
    return slice;
}

void FrameParser::readFrameLength(char* buf, int offset) {
    this->frameLength = (size_t) readFrameSize(buf, offset);

    if (this->frameLength <= 16) {
        fprintf(stderr, "Got unexpected really small frame\n");
    }
}

// TODO: audit against out of bound reads
uint16_t readFrameSize(char* buf, int offset) {
    uint16_t result = (uint16_t)(
        ((unsigned char)buf[offset] << 8) |
        ((unsigned char)buf[offset + 1])
    );

    return result;
};

char* bufCopySlice(char* buf, int start, int end) {
    // TODO do not noob malloc.
    char* newBuffer = (char*) malloc(end - start);
    char* sourceBuffer = (char*) &buf[start];

    strncpy( newBuffer, sourceBuffer, end - start);

    return newBuffer;
}

}
