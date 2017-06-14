#include <csignal>
#include "BladePoolServer.h"
#include "src/utils/logging.h"

static const uint64_t GB = (1024*1024*1024);
static const int PORT = 12345;
static const int initial_buffer_size = 50;

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

auto main() -> int {
    cirrus::LOG<cirrus::INFO>("Starting RDMA server in port: ", PORT);

    cirrus::BladePoolServer server(PORT, 10 * GB);

    server.init();

    cirrus::LOG<cirrus::INFO>("Running server's loop");
    server.loop();

    return 0;
}
