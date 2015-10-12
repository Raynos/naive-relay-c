#include <iostream>
#include "deps/libuv/include/uv.h"
#include "naive-relay.h"

int main(/*int argc, char *argv[]*/) {
    uv_loop_t *loop = uv_default_loop();

    const char *serverHost = "0.0.0.0";
    int serverPort = 7000;

    tchannel::NaiveRelay relay = tchannel::NaiveRelay(loop);

    relay.listen(serverPort, serverHost);

    std::cerr << "Listening on adress " << relay.hostPort << std::endl;

    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
