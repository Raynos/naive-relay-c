#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "deps/libuv/include/uv.h"

namespace tchannel {

uint16_t readFrameSize(char *buf, int offset);
char * bufCopySlice(char *buf, int start, int end);

FrameParser::FrameParser() {
    this->remainderLength = 0;
    this->frameLength = 0;
}

void FrameParser::write(char *buf, ssize_t nread) {
    if (this->frameLength == 0) {
        this->frameLength = (int) readFrameSize(buf, 0);
    }

    if (this->frameLength <= 16) {
        fprintf(stderr, "Got unexpected really small frame\n");
    }

    int bufferLength = nread;
    int totalLength = this->remainderLength + bufferLength;

    if (this->frameLength == totalLength) {
        char* lastBuffer = bufCopySlice(buf, 0, nread)
        this->pushFrameBuffer(lastBuffer);
        return;
    }

    if (this->frameLength > totalLength) {
        this->addRemainder(bufCopySlice(buf, 0, nread), nread);
    }

    int startOfBuffer = 0;

    while (this->frameLength <= totalLength) {
        int endOfBuffer = this->frameLength - this->remainderLength;

        char *lastBuffer = bufCopySlice(buf, startOfBuffer, endOfBuffer);
        this->pushFrameBuffer(lastBuffer);

        if (endOfBuffer == bufferLength) {
            return;
        }

        buf = bufCopySlice(buf, endOfBuffer, bufferLength);
        totalLength = bufferLength - endOfBuffer;
        this->frameLength = (int) readFrameSize(buf, startOfBuffer);
    }

    if (bufferLength) {
        this->addRemainder(buf);
    }
}

bool FrameParser::hasFrameBuffers() {
    return this->frameBuffers.size() != 0;
}

char * FrameParser::getFrameBuffer() {
    assert(this->frameBuffers.size() > 0 && "frameBuffers is empty");

    char *last = this->frameBuffers.back();
    this->frameBuffers.pop_back();
    return last;
}

void FrameParser::pushFrameBuffer(char *buf) {
    char *frameBuffer = this->concatRemainder(buf);
    this->frameBuffers.push_back(frameBuffer);
    this->frameLength = 0;
}

void FrameParser::addRemainder(char *buf, int size) {
    this->remainder.push_back(buf);
    this->remainderLength += size;
}

char * FrameParser::concatRemainder(char *buf) {
    if (this->remainderLength == 0) {
        return buf;
    }

    return (char*) NULL;
}

uint16_t readFrameSize(char *buf, int offset) {
    uint16_t result = (uint16_t)(
        ((unsigned char)buf[offset] << 8) |
        ((unsigned char)buf[offset + 1])
    );

    return result;
};

char * bufCopySlice(char *buf, int start, int end) {
    // TODO do not noob malloc.
    char *newBuffer = (char*) malloc(end - start);
    char *sourceBuffer = (char*) &buf[start];

    strncpy( newBuffer, sourceBuffer, end - start);

    return newBuffer;
}

}
