#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "deps/libuv/include/uv.h"

#define DEFAULT_BACKLOG 128

uv_loop_t *loop;

void on_new_connection(uv_stream_t *server, int status) {
    int r;
    assert(status >= 0 && "libuv incoming connection error");

    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);


    r = uv_accept(server, (uv_stream_t*) client);
    if (r) {
        fprintf(stderr, "Could not accept socket %d", r);
        uv_close((uv_handle_t*) client, NULL);
        // free(client)
    } else {
        fprintf(stderr, "Got incoming socket\n");
    }
}

int main(int argc, char *argv[]) {
    int r;
    loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    const char* serverHost = "0.0.0.0";
    int serverPort = 7000;
    struct sockaddr_in serverAddress;

    r = uv_ip4_addr(serverHost, serverPort, &serverAddress);
    assert(!r && "could not allocate address");

    r = uv_tcp_bind(&server, (struct sockaddr*) &serverAddress, 0);
    assert(!r && "could not bind to server socket");

    r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, on_new_connection);
    assert(!r && "could not listen to tcp server");

    fprintf(stderr, "Listening on address %s:%d\n", serverHost, serverPort);

    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
