#include "deps/libuv/include/uv.h"

namespace tchannel {

void on_new_connection(uv_stream_t *server, int status);

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
