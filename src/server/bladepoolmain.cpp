/* Copyright 2016 Joao Carreira */

#include <csignal>
#include "BladePoolServer.h"
#include "third_party/easylogging++.h"

static const uint64_t GB = (1024*1024*1024);

static const int PORT = 12345;

INITIALIZE_EASYLOGGINGPP

void ctrlc_handler(int sig_num) {
    LOG(ERROR) << "Caught CTRL-C. sig_num: " << sig_num;
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
    LOG(INFO) << "Starting RDMA server in port: " << PORT;

    sirius::BladePoolServer server(PORT, 10 * GB);

    server.init();

    LOG(INFO) << "Running server's loop";
    server.loop();

    return 0;
}

