#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include "deps/libuv/include/uv.h"
#include "naive-relay.h"

static std::vector<std::string> split(std::string str, char delim);

int main(int argc, char *argv[]) {
    uv_loop_t *loop = uv_default_loop();

    assert(argc == 5 && "Expected to get 5 arguments");

    argv = uv_setup_args(argc, argv);

    const char *serverHost = argv[1];
    char* end;
    int serverPort = std::strtol(argv[2], &end, 10);
    std::string destinationsCSV = std::string(argv[3]);
    bool printRPS = std::strtol(argv[4], &end, 10) != 0;
    (void) printRPS; // TODO use printRPS


    auto destinationHostPorts = split(destinationsCSV, ',');
    std::vector<struct sockaddr_in> destinations;

    for (std::string hostPort : destinationHostPorts) {
        struct sockaddr_in addr;
        std::vector<std::string> parts = split(hostPort, ':');
        std::string host = parts[0];
        std::size_t end;
        int port = std::stoi(parts[1], &end, 10);

        uv_ip4_addr(host.c_str(), port, &addr);
        destinations.push_back(addr);
    }

    tchannel::NaiveRelay relay = tchannel::NaiveRelay(
        loop, destinations, serverPort, serverHost
    );

    relay.listen();

    std::cerr << "INFO: Listening on address " << relay.hostPort << std::endl;

    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}

static std::vector<std::string> split(std::string str, char delim) {
    std::stringstream stringStream(str);
    std::string item;
    std::vector<std::string> items;

    while (std::getline(stringStream, item, delim)) {
        items.push_back(item);
    }
    return items;
}
