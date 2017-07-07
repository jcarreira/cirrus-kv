#include <sstream>
#include <iostream>
#include "server/TCPServer.h"

const int port = 12345;
const int queue_len = 10;

/* Start a server that runs and prints messages received. */
auto main(int argc, char *argv[]) -> int {
    uint64_t pool_size;
    if (argc == 1) {
        pool_size = 10 * GB;
    } else if (argc == 2) {
        std::istringstream iss(argv[1]);
        if (!(iss >> pool_size)) {
            std::cout << "Pool size in invalid format." << std::endl;
        }
    } else {
        std::cout << "Pass desired poolsize in bytes" << std::endl;
    }
    cirrus::TCPServer server(port, queue_len, pool_size);
    server.init();
    server.loop();
    return 0;
}
