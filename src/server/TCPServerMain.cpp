#include <sstream>
#include <iostream>
#include "server/TCPServer.h"
#include "utils/logging.h"

const int port = 12345;
const int max_fds = 100;
static const uint64_t MB = (1024 * 1024);
static const uint64_t GB = (1024 * MB);

void print_arguments() {
    std::cout
        << "Error: ./tcpservermain"
        << " [pool_size=10] [backend_type=Memory]"
        << std::endl
        << " pool_size in MB" << std::endl
        << std::endl;
}

/**
 * Starts a TCP based key value store server. Accepts the pool size as
 * a command line argument. This specifies how large a memory pool will be
 * available to the server. Pool size defaults to 10 GB if not specified.
 */
auto main(int argc, char *argv[]) -> int {
    uint64_t pool_size = 10 * GB;
    std::string backend_type = "Memory";
    std::string storage_path = "/tmp/cirrus_storage";

    switch (argc) {
        case 4:
            {
                storage_path = argv[3];
#if __GNUC__ >= 7
                [[fallthrough]];
#endif
            }
        case 3:
            {
                if (strcmp(argv[2], "Memory") && strcmp(argv[2], "Storage")) {
                    throw std::runtime_error("Wrong backend type");
                }
                backend_type = argv[2];
#if __GNUC__ >= 7
                [[fallthrough]];
#endif
            }
        case 2:
            {
                std::istringstream iss(argv[1]);
                if (!(iss >> pool_size)) {
                    std::cout << "Pool size in invalid format." << std::endl;
                    return -1;
                }

                pool_size *= MB;
#if __GNUC__ >= 7
                [[fallthrough]];
#endif
            }
        case 1:
            break;
        default:
            print_arguments();
            throw std::runtime_error("Wrong number of arguments");
    }

    // Instantiate the server
    cirrus::LOG<cirrus::INFO>(
            "Starting TCPServer in port: ", port,
            " with memory: ", pool_size);
    cirrus::TCPServer server(port, pool_size, backend_type,
                             storage_path, max_fds);
    // Initialize the server
    server.init();
    // Loop the server and listen for clients. Act on requests
    cirrus::LOG<cirrus::INFO>("Running server's loop");
    server.loop();
    return 0;
}
