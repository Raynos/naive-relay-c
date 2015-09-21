#include "deps/libuv/include/uv.h"

namespace tchannel {

class FrameParser {
public:
    FrameParser();
    ~FrameParser();

    void write(char* buf, ssize_t nread);
    bool hasFrameBuffers();
    char* getFrameBuffer();

private:
    
};

}
