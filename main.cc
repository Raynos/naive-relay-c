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

    const char *serverHost = argv[1];
    char* end;
    int serverPort = std::strtol(argv[2], &end, 10);
    std::string destinationsCSV = std::string(argv[3]);
    std::vector<std::string> destinations = split(destinationsCSV, ',');
    bool printRPS = std::strtol(argv[4], &end, 10) != 0;
    (void) printRPS; // TODO use printRPS

    tchannel::NaiveRelay relay = tchannel::NaiveRelay(loop, destinations);

    relay.listen(serverPort, serverHost);

    std::cerr << "Listening on adress " << relay.hostPort << std::endl;

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
