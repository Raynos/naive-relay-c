#include <vector>
#include "deps/libuv/include/uv.h"

namespace tchannel {

class FrameParser {
public:
    FrameParser();
    // ~FrameParser();

    void write(const uv_buf_t *buf, ssize_t nread);
    bool hasFrameBuffers();
    char* getFrameBuffer();

private:
    void addRemainder(char *buf, int size);
    char* concatRemainder(char *buf);
    void pushFrameBuffer(char* buf);

    std::vector<char*> remainder;
    std::vector<char*> frameBuffers;
    int remainderLength;
    int frameLength;
};

}
