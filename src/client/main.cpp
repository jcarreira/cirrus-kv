/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <iostream>
#include <csignal>
#include <memory>
#include <sstream>
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

char data[1000];

void test1() {
    snprintf(data, sizeof(data), "%s", "WRONG");

    LOG(INFO) << "Starting RDMA server in port: " << PORT;

    sirius::BladeClient client1, client2;

    client1.connect("10.10.49.87", PORT);
    client2.connect("10.10.49.87", PORT);

    LOG(INFO) << "Connected to blade";

    sirius::AllocRec alloc1 = client1.allocate(1 * MB);
    sirius::AllocRec alloc2 = client2.allocate(1 * GB);

    LOG(INFO) << "Received allocation 1. id: " << alloc1->alloc_id;
    LOG(INFO) << "Received allocation 2. id: " << alloc2->alloc_id;

    srand (time(NULL));
    std::ostringstream oss;
    for (int i = 0; i < 1; ++i) {
        sirius::TimerFunction tf("client1.write");
        oss << "data" << rand();
        LOG(INFO) << "Writing " << oss.str().c_str();
        client1.write(alloc1, 0, oss.str().size(), oss.str().c_str());
    }
    client2.write(alloc2, 0, 5, "data2");

    LOG(INFO) << "Old data: " << data;
    client1.read(alloc1, 0, oss.str().size(), data);
    LOG(INFO) << "Received data 1: " << data;

    client2.read(alloc2, 0, 5, data);
    LOG(INFO) << "Received data 2: " << data;

}

// test bandwidth utilization
void test2() {
    sirius::BladeClient client;

    client.connect("10.10.49.87", PORT);
    LOG(INFO) << "Connected to blade";

    uint64_t mem_size = 1 * GB;

    char* data = (char*)malloc(mem_size);
    if (!data)
        exit(-1);

    memset(data, 0, mem_size);
    data[0] = 'Y';

    sirius::AllocRec alloc1 = client.allocate(1 * GB); // currently ignored
    LOG(INFO) << "Received allocation 1. id: " << alloc1->alloc_id;
    
    // small write to force creation of pool    
    {
        sirius::TimerFunction tf("Timing 1byte write", true);
        client.write(alloc1, 0, 1, data);
    }

    sleep(1);

    {
        sirius::TimerFunction tf("Timing write", true);
        client.write_sync(alloc1, 0, mem_size, data);
    }

    {
        sirius::TimerFunction tf("Timing read", true);
        client.read(alloc1, 0, mem_size, data);
        std::cout << "data[0]: " << data[0] << std::endl;
    }
}

int main() {
    test2();
    return 0;
}

