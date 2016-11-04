/* Copyright 2016 Joao Carreira */

#include <csignal>
#include "BladePoolServer.h"
#include "src/utils/logging.h"

static const uint64_t GB = (1024*1024*1024);

static const int PORT = 12345;

void ctrlc_handler(int sig_num) {
    LOG(ERROR) << "Caught CTRL-C. sig_num: " << sig_num << std::endl;
    exit(EXIT_FAILURE);
}

void set_ctrlc_handler() {
    struct sigaction sig_int_handler;

    sig_int_handler.sa_handler = ctrlc_handler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;

    sigaction(SIGINT, &sig_int_handler, NULL);
}

int main() {
    LOG(INFO) << "Starting RDMA server in port: " << PORT << std::endl;

    sirius::BladePoolServer server(PORT, 10 * GB);

    server.init();

    LOG(INFO) << "Running server's loop" << std::endl;
    server.loop();

    return 0;
}

