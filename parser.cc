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

void FrameParser::write(char* buf, size_t size) {
    if (this->frameLength == 0) {
        this->frameLength = (size_t) readFrameSize(buf, 0);
    }

    if (this->frameLength <= 16) {
        fprintf(stderr, "Got unexpected really small frame\n");
    }

    size_t bufferLength = size;
    size_t totalLength = this->remainderLength + bufferLength;

    if (this->frameLength == totalLength) {
        char* lastBuffer = bufCopySlice(buf, 0, size);
        this->pushFrameBuffer(lastBuffer, size);
        return;
    }

    if (this->frameLength > totalLength) {
        this->addRemainder(bufCopySlice(buf, 0, size), size);
    }

    size_t startOfBuffer = 0;

    while (this->frameLength <= totalLength) {
        size_t endOfBuffer = this->frameLength - this->remainderLength;

        char* lastBuffer = bufCopySlice(buf, startOfBuffer, endOfBuffer);
        this->pushFrameBuffer(lastBuffer, endOfBuffer - startOfBuffer);

        if (endOfBuffer == bufferLength) {
            return;
        }

        buf = bufCopySlice(buf, endOfBuffer, bufferLength);
        totalLength = bufferLength - endOfBuffer;
        this->frameLength = (size_t) readFrameSize(buf, startOfBuffer);
    }

    if (bufferLength) {
        this->addRemainder(buf, bufferLength);
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
