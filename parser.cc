#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "deps/libuv/include/uv.h"

namespace tchannel {

char* bufCopySlice(char* buf, int start, int end);

BufferSlice::BufferSlice() {
    this->buf = nullptr;
    this->start = (size_t) 0;
    this->end = (size_t) 0;
    this->length = (size_t) 0;
}

BufferSlice::BufferSlice(char* buf, size_t start, size_t end) {
    this->buf = buf;
    this->start = start;
    this->end = end;
    this->length = end - start;
}

FrameParser::FrameParser() {
    this->remainderLength = 0;
    this->frameLength = 0;
    this->reader = Buffer::BufferReader();
}

// buf will be freed by caller.
void FrameParser::write(char* buf, size_t size) {
    size_t totalBufferLength = size;

    if (this->frameLength == 0) {
        this->readFrameLength(buf, 0, totalBufferLength);
    }

    size_t totalLength = this->remainderLength + totalBufferLength;

    if (this->frameLength == totalLength) {
        this->pushFrameBuffer(buf, 0, size);
        return;
    }

    if (this->frameLength > totalLength) {
        this->addRemainder(buf, 0, size);
        return;
    }

    size_t startOfBuffer = 0;

    while (this->frameLength <= totalLength) {
        size_t amountToRead = this->frameLength - this->remainderLength;

        this->pushFrameBuffer(buf, startOfBuffer, startOfBuffer + amountToRead);

        if (startOfBuffer + amountToRead == totalBufferLength) {
            return;
        }

        startOfBuffer = startOfBuffer + amountToRead;
        totalLength = totalBufferLength - startOfBuffer;
        this->readFrameLength(buf, startOfBuffer, totalBufferLength);
    }

    if (startOfBuffer < totalBufferLength) {
        this->addRemainder(buf, startOfBuffer, totalBufferLength);
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

void FrameParser::pushFrameBuffer(char* buf, size_t start, size_t end) {
    // TODO free `rawFrameBuffer`
    char* rawFrameBuffer = bufCopySlice(buf, start, end);

    BufferSlice frameBuffer = this->concatRemainder(
        rawFrameBuffer, end - start
    );
    this->frameBuffers.push_back(frameBuffer);
    this->frameLength = 0;
}

void FrameParser::addRemainder(char* buf, size_t start, size_t end) {
    char* remainderBuffer = bufCopySlice(buf, start, end);

    BufferSlice slice = BufferSlice(remainderBuffer, 0, end - start);
    this->remainder.push_back(slice);
    this->remainderLength += slice.length;
}

BufferSlice FrameParser::concatRemainder(char* buf, size_t size) {
    BufferSlice slice;

    if (this->remainderLength == 0) {
        slice = BufferSlice(buf, 0, size);
        return slice;
    }

    // TODO remove noob malloc.
    char* rawBuffer = (char*) malloc(this->remainderLength + size);

    char* destBuffer = rawBuffer;
    for (uint i = 0; i < this->remainder.size(); i++) {
        BufferSlice tempSlice = this->remainder[i];
        assert(tempSlice.start == 0 && "Cannot concat non-complete buffers");

        memcpy(destBuffer, tempSlice.buf, tempSlice.length);
        destBuffer = (char*) destBuffer + tempSlice.length;

        free(tempSlice.buf);
    }

    slice = BufferSlice(rawBuffer, 0, this->remainderLength + size);

    this->remainder.clear();
    this->remainderLength = 0;

    return slice;
}

void FrameParser::readFrameLength(char* buf, int offset, size_t length) {
    this->reader = Buffer::BufferReader(buf, length);
    this->reader.Skip((size_t) offset);

    this->frameLength = this->reader.ReadUint16BE();

    if (this->reader.Error()) {
        assert("Out of bounds read into buffer");
    }

    if (this->frameLength <= 16) {
        fprintf(stderr, "Got unexpected really small frame\n");
    }
}

char* bufCopySlice(char* buf, int start, int end) {
    // TODO do not noob malloc.
    char* newBuffer = (char*) malloc(end - start);
    char* sourceBuffer = (char*) &buf[start];

    memcpy( newBuffer, sourceBuffer, end - start);

    return newBuffer;
}

}
