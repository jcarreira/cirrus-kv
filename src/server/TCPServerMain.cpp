#include <sstream>
#include <iostream>
#include "server/TCPServer.h"
#include "utils/logging.h"

const int port = 12345;
const int max_fds = 100;
static const uint64_t GB = (1024*1024*1024);

/**
 * Starts a TCP based key value store server. Accepts the pool size as
 * a command line argument. This specifies how large a memory pool will be
 * available to the server. Pool size defaults to 10 GB if not specified.
 */
auto main(int argc, char *argv[]) -> int {
    uint64_t pool_size;
    // Parse arguments
    if (argc == 1) {
        // Default of 10 gigabytes
        pool_size = 10 * GB;
    } else if (argc == 2) {
        std::istringstream iss(argv[1]);
        if (!(iss >> pool_size)) {
            std::cout << "Pool size in invalid format." << std::endl;
            return -1;
        }
    } else {
        std::cout << "Error: ./tcpservermain [pool_size]" << std::endl;
        return -1;
    }
    // Instantiate the server
    cirrus::LOG<cirrus::INFO>("Starting TCPServer in port: ", port);
    cirrus::TCPServer server(port, pool_size, max_fds);
    // Initialize the server
    server.init();
    // Loop the server and listen for clients. Act on requests
    cirrus::LOG<cirrus::INFO>("Running server's loop");
    server.loop();
    return 0;
}
