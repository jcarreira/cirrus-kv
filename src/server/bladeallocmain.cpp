#include <csignal>
#include <sstream>
#include <iostream>
#include "BladeAllocServer.h"
#include "utils/logging.h"

static const uint64_t GB = (1024*1024*1024);

static const int PORT = 12345;

void ctrlc_handler(int sig_num) {
    cirrus::LOG<cirrus::ERROR>("Caught CTRL-C. sig_num: ", sig_num);
    exit(EXIT_FAILURE);
}

void set_ctrlc_handler() {
    struct sigaction sig_int_handler;

    sig_int_handler.sa_handler = ctrlc_handler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;

    sigaction(SIGINT, &sig_int_handler, nullptr);
}

/**
 * Starts an RDMA based key value store server. Accepts the pool size as
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
        }
    } else {
        std::cout << "Pass desired poolsize in bytes" << std::endl;
    }

    cirrus::LOG<cirrus::INFO>("Starting BladeAllocServer in port: ", PORT);

    cirrus::BladeAllocServer server(PORT, pool_size);

    server.init();

    cirrus::LOG<cirrus::INFO>("Running server's loop");
    server.loop();

    return 0;
}
