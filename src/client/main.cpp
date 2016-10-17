/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <iostream>
#include <csignal>
#include <memory>
#include <string>
#include "src/client/BladeClient.h"
#include "src/common/AllocationRecord.h"
#include "src/utils/easylogging++.h"
//#include "src/common/config.h"
#include "src/utils/TimerFunction.h"

const char PORT[] = "12345";
static const uint64_t MB = (1024*1024);
static const uint64_t GB = (1024*MB);

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

char data[100] = {0};

int main() {
    snprintf(data, sizeof(data), "%s", "WRONG");

    LOG(INFO) << "Starting RDMA server in port: " << PORT;

    sirius::BladeClient client1, client2;

    client1.connect("10.10.49.87", PORT);
    //client2.connect("10.10.49.87", PORT);

    LOG(INFO) << "Connected to blade";

    sirius::AllocRec alloc1 = client1.allocate(60 * GB);
    //sirius::AllocRec alloc2 = client2.allocate(1 * GB);

    LOG(INFO) << "Received allocation 1. id: " << alloc1->alloc_id;
    //LOG(INFO) << "Received allocation 2. id: " << alloc2->alloc_id;

    for (int i = 0; i < 100; ++i) {
        sirius::TimerFunction tf("client1.write");
        client1.write(alloc1, 0, 5, "data1");
    }
    //client2.write(alloc2, 0, 5, "data2");

    LOG(INFO) << "Old data: " << data;
    client1.read(alloc1, 0, 5, data);
    LOG(INFO) << "Received data 1: " << data;

    //client2.read(alloc2, 0, 5, data);
    LOG(INFO) << "Received data 2: " << data;

    return 0;
}

