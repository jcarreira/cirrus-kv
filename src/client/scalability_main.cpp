/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <iostream>
#include <csignal>
#include <memory>
#include <string>
#include "src/client/BladeClient.h"
#include "src/common/AllocationRecord.h"
#include "src/utils/logging.h"
#include "src/utils/TimerFunction.h"

const char PORT[] = "12345";
static const uint64_t MB = (1024*1024);
static const uint64_t GB = (1024*MB);

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

char data[100] = {0};

auto main() -> int {
    snprintf(data, sizeof(data), "%s", "WRONG");

    sirius::LOG<sirius::INFO>("Starting RDMA server in port: ", PORT);

    sirius::BladeClient client1, client2;

    client1.connect("10.10.49.83", PORT);

    for (int i = 1; i < 1024 * 10; i += (1024*10 / 50)) {
        std::cout << "Allocating " << i << "MB" << std::endl;
        sirius::AllocationRecord alloc1 = client1.allocate(i * MB);

        sirius::LOG<sirius::INFO>("Received allocation 1. id: ", alloc1.alloc_id);

        // average latencies
        uint64_t elapsed_cum = 0;
        for (int j = 0; j < 20; ++j) {
            sirius::TimerFunction tf("client1.write");
            client1.write_sync(alloc1, 0, 5, "data1");
            elapsed_cum += tf.getUsElapsed();
        }

        std::cout << "Average latency: " << elapsed_cum << std::endl;
        usleep(100000);
    }
    return 0;
}

