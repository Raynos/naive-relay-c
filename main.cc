#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <assert.h>
#include "deps/libuv/include/uv.h"
#include "naive-relay.h"

int main(/*int argc, char *argv[]*/) {
    uv_loop_t *loop = uv_default_loop();

    const char *serverHost = "0.0.0.0";
    int serverPort = 7000;

    tchannel::NaiveRelay relay = tchannel::NaiveRelay(loop);

    relay.listen(serverPort, serverHost);

    fprintf(stderr, "Listening on address %s\n", relay.hostPort);

    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
