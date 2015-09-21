#include "deps/libuv/include/uv.h"

namespace tchannel {

class NaiveRelay {
public:
    NaiveRelay(uv_loop_t *loop);
    ~NaiveRelay();

    void listen(int serverPort, const char *serverHost);
    void onNewConnection();

    char hostPort[128];

private:
    uv_tcp_t *server;
    uv_loop_t *loop;

};

}
