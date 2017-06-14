#include <csignal>
#include "src/server/ResourceAllocator.h"
#include "src/utils/logging.h"

static const int PORT = 12346;

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
    cirrus::LOG<cirrus::INFO>("Starting resource allocator in port: ", PORT);

    cirrus::ResourceAllocator resalloc(PORT);

    cirrus::LOG<cirrus::INFO>("Running allocator init()");
    resalloc.init();

    cirrus::LOG<cirrus::INFO>("Running resource allocator loop");
    resalloc.loop();

    return 0;
}

