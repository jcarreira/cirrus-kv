/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <iostream>
#include <csignal>
#include <memory>
#include <sstream>
#include <cstring>
#include "src/client/BladeClient.h"
#include "src/common/AllocationRecord.h"
#include "src/utils/logging.h"
#include "src/authentication/AuthenticationToken.h"
#include "src/client/AuthenticationClient.h"
#include "src/utils/TimerFunction.h"

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
    const char* to_send = "SIRIUS_DDC";
    snprintf(data, sizeof(data), "%s", "WRONG");

    sirius::BladeClient client;
    client.connect("10.10.49.83", PORT);

    LOG(INFO) << "Connected to blade";
    sirius::AllocRec alloc1 = client.allocate(1 * MB);

    LOG(INFO) << "Received allocation 1. id: " << alloc1->alloc_id
        << " remote_addr: " << alloc1->remote_addr
        << " peer_rkey: " << alloc1->peer_rkey;

    client.write_sync(alloc1, 0, std::strlen(to_send), to_send);
    client.read_sync(alloc1, 0, std::strlen(to_send), data);
    if (strncmp(data, to_send, std::strlen(to_send)) != 0)
        exit(-1);
    
    sirius::AllocRec alloc2 = client.allocate(2 * MB);
    LOG(INFO) << "Received allocation 2. id: " << alloc2->alloc_id
        << " remote_addr: " << alloc2->remote_addr
        << " peer_rkey: " << alloc2->peer_rkey;

    client.read_sync(alloc2, 0, std::strlen(to_send), data);
    LOG(INFO) << "Received data 2: " << data;
}

void test_1_client() {
    char data[1000];
    const char* to_send = "SIRIUS_DDC";

    snprintf(data, sizeof(data), "%s", "WRONG");

    LOG(INFO) << "Connecting to server in port: " << PORT;

    sirius::BladeClient client1;
    client1.connect("10.10.49.83", PORT);

    LOG(INFO) << "Connected to blade";

    sirius::AllocRec alloc1 = client1.allocate(1 * MB);

    LOG(INFO) << "Received allocation 1. id: " << alloc1->alloc_id;

    client1.write_sync(alloc1, 0, std::strlen(to_send), to_send);

    LOG(INFO) << "Old data: " << data;
    client1.read_sync(alloc1, 0, std::strlen(to_send), data);
    LOG(INFO) << "Received data 1: " << data;
}

void test_2_clients() {
    char data[1000];
    snprintf(data, sizeof(data), "%s", "WRONG");

    LOG(INFO) << "Connecting to server in port: " << PORT;

    sirius::BladeClient client1, client2;

    client1.connect("10.10.49.83", PORT);
    client2.connect("10.10.49.87", PORT);

    LOG(INFO) << "Connected to blade";

    sirius::AllocRec alloc1 = client1.allocate(1 * MB);
    sirius::AllocRec alloc2 = client2.allocate(1 * GB);

    LOG(INFO) << "Received allocation 1. id: " << alloc1->alloc_id;
    LOG(INFO) << "Received allocation 2. id: " << alloc2->alloc_id;

    unsigned int seed = 42;
    std::ostringstream oss;
    {
        oss << "data" << rand_r(&seed);
        LOG(INFO) << "Writing " << oss.str().c_str();
        sirius::TimerFunction tf("client1.write");
        client1.write_sync(alloc1, 0, oss.str().size(), oss.str().c_str());
    }

    client2.write_sync(alloc2, 0, 5, "data2");

    LOG(INFO) << "Old data: " << data;
    client1.read_sync(alloc1, 0, oss.str().size(), data);
    LOG(INFO) << "Received data 1: " << data;

    client2.read_sync(alloc2, 0, 5, data);
    LOG(INFO) << "Received data 2: " << data;
}

// test bandwidth utilization
void test_performance() {
    sirius::BladeClient client;
    client.connect("10.10.49.87", PORT);

    LOG(INFO) << "Connected to blade";

    uint64_t mem_size = 1 * GB;

    char* data = reinterpret_cast<char*>(malloc(mem_size));
    if (!data)
        exit(-1);

    memset(data, 0, mem_size);
    data[0] = 'Y';

    sirius::AllocRec alloc1 = client.allocate(1 * GB);  // currently ignored
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

void test_authentication() {
    std::string controller_address = "10.10.49.87";
    std::string controller_port = "12346";

    char data[1000];
    const char* to_send = "SIRIUS_DDC";

    snprintf(data, sizeof(data), "%s", "WRONG");

    LOG(INFO) << "Connecting to server in port: " << PORT;

    sirius::BladeClient client;

    sirius::AuthenticationToken token(false);
    client.authenticate(controller_address, controller_port, token);

    client.connect("10.10.49.87", PORT);

    LOG(INFO) << "Connected to blade";

    sirius::AllocRec alloc1 = client.allocate(1 * MB);

    LOG(INFO) << "Received allocation 1. id: " << alloc1->alloc_id;

    srand(time(NULL));

    client.write_sync(alloc1, 0, std::strlen(to_send), to_send);

    LOG(INFO) << "Old data: " << data;
    client.read_sync(alloc1, 0, std::strlen(to_send), data);
    LOG(INFO) << "Received data 1: " << data;
}

void test_destructor() {
    std::string controller_address = "10.10.49.88";
    std::string controller_port = "12346";

    sirius::BladeClient client;

    sirius::AuthenticationToken token(false);
    client.authenticate(controller_address, controller_port, token);
}

int main() {
    //test_1_client();
    // test_2_clients();
    // test_performance();
    // test_authentication();
    // test_destructor();
    test_allocation();
    return 0;
}

