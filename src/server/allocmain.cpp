/* Copyright 2016 Joao Carreira */

#include <csignal>
#include "src/server/ResourceAllocator.h"
#include "src/utils/logging.h"

static const int PORT = 12346;

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
    sirius::LOG<sirius::INFO>("Starting resource allocator in port: ", PORT);

    sirius::ResourceAllocator resalloc(PORT);

    sirius::LOG<sirius::INFO>("Running allocator init()");
    resalloc.init();

    sirius::LOG<sirius::INFO>("Running resource allocator loop");
    resalloc.loop();

    return 0;
}

