#include <sstream>
#include <iostream>
#include "coordinator/Coordinator.h"
#include "utils/logging.h"

const int port = 12345;
const int max_fds = 100;
static const uint64_t GB = (1024*1024*1024);

/**
 * Starts a TCP based Coordinator
 */
auto main(int argc, char *argv[]) -> int {

    // Parse arguments
    if (argc == 1) {
    } else if (argc == 2) {
        std::cout << argv[2] << std::endl;
    } else {
        std::cout << "Error: ./coordinator" << std::endl;
        return -1;
    }

    // Instantiate the coordinator
    cirrus::LOG<cirrus::INFO>("Starting Coordinator in port: ", port);
    cirrus::Coordinator coordinator(port, max_fds);
    // Initialize the coordinator
    coordinator.init();
    // Loop the coordinator and listen for clients. Act on requests
    cirrus::LOG<cirrus::INFO>("Running coordinator's loop");
    coordinator.loop();
    return 0;
}
