#include "parser.h"
#include "deps/libuv/include/uv.h"

namespace tchannel {

class NaiveRelay;

class RelayConnection {
public:
    RelayConnection(
        uv_loop_t *loop, tchannel::NaiveRelay *relay, const char *direction
    );
    ~RelayConnection();

    void accept(uv_stream_t *server);
    void readStart();
    void onSocketRead(ssize_t nread, const uv_buf_t *buf);

    uv_tcp_t *socket;
    char direction[16];

private:
    tchannel::NaiveRelay *relay;
    uv_loop_t *loop;
    tchannel::FrameParser *parser;
    /* parser */
    /* idCounter */
    /* guid */
    /* outReq dictionary */
    /* initialized */
    /* frameQueue */
};

}
