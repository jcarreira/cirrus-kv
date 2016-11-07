/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <iostream>
#include <csignal>
#include <memory>
#include <sstream>
#include <string>
#include <cstring>
#include "src/client/BladeFileClient.h"
#include "src/common/FileAllocationRecord.h"
#include "src/authentication/AuthenticationToken.h"
#include "src/client/AuthenticationClient.h"
#include "src/utils/TimerFunction.h"
#include "src/utils/logging.h"

const char PORT[] = "12345";
static const uint64_t MB = (1024*1024);
static const uint64_t GB = (1024*MB);

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

void test_allocation() {
    char data[1000];
    sirius::BladeFileClient client;
    client.connect("10.10.49.83", PORT);

    LOG(INFO) << "Connected to blade" << std::endl;
    sirius::FileAllocRec alloc1 = client.allocate("/tmp/test", 1 * MB);

    LOG(INFO) << "Received allocation 1"
        << " remote_addr: " << alloc1->remote_addr
        << " peer_rkey: " << alloc1->peer_rkey << std::endl;

    const char* to_send = "SIRIUS_DDC";
    client.write_sync(alloc1, 0, strlen(to_send), to_send);
    client.read_sync(alloc1, 0, strlen(to_send), data);
    LOG(INFO) << "Received data 1: " << data << std::endl;
    if (strncmp(data, to_send, strlen(to_send)) != 0)
        exit(-1);
    
    sirius::FileAllocRec alloc2 = client.allocate("/tmp/test", 2 * MB);
    LOG(INFO) << "Received allocation 2."
        << " remote_addr: " << alloc2->remote_addr
        << " peer_rkey: " << alloc2->peer_rkey << std::endl;

    client.read_sync(alloc2, 0, strlen(to_send), data);
    LOG(INFO) << "Received data 2: " << data << std::endl;
    
    client.write_sync(alloc1, 2, 2, "TT");
    client.read_sync(alloc1, 0, strlen(to_send), data);
    LOG(INFO) << "Received data 3: " << data << std::endl;
}


int main() {
    test_allocation();
    return 0;
}
