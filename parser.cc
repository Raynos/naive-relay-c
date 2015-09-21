#include "parser.h"
#include "deps/libuv/include/uv.h"

namespace tchannel {

FrameParser::FrameParser() {

}

FrameParser::~FrameParser() {

}

void FrameParser::write(char* buf, ssize_t nread) {
    (void) buf;
    (void) nread;
}

bool FrameParser::hasFrameBuffers() {
    return false;
}

char* FrameParser::getFrameBuffer() {
    return NULL;
}

}
