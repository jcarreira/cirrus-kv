/* Copyright 2016 Joao Carreira */

#include <csignal>
#include "BladeFileAllocServer.h"
#include "src/utils/logging.h"

static const uint64_t GB = (1024*1024*1024);
static const int PORT = 12345;

void ctrlc_handler(int sig_num) {
    sirius::LOG<sirius::ERROR>("Caught CTRL-C. sig_num: ", sig_num);
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
    sirius::LOG<sirius::INFO>("Starting RDMA server in port: ", PORT);

    sirius::BladeFileAllocServer server(PORT, 20 * GB);

    server.init();

    sirius::LOG<sirius::INFO>("Running server's loop");
    server.loop();

    return 0;
}

