/* Copyright Joao Carreira 2016 */

#include <stdlib.h>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <cctype>
#include <chrono>
#include <thread>
#include <memory>
#include <random>

#include "src/client/BladeFileClient.h"
#include "src/utils/Time.h"

static const uint64_t GB = (1024*1024*1024);
static const uint64_t MB = (1024*1024);
static const int N_THREADS = 2;
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1024;
char* data = "TESTE"; 

struct Dummy {
    char data[SIZE];
    int id;
};

void test_file() {
    sirius::BladeFileClient client;
    client.connect(IP, PORT);

    sirius::FileAllocRec allocRec = client.allocate("/tmp/teste", SIZE);

    sirius::RDMAMem mem;
    char input[10] = "TESTE";
    auto fut = client.write_async(allocRec, 0, 6, input, mem);

    std::cout << "Waiting for write async to finish" << std::endl;
    fut->wait();

    auto fut2 = client.read_async(allocRec, 0, 6, input, mem);

    fut2->wait();

    if (strcmp(input, "TESTE") != 0)
        throw std::runtime_error("Test error");

    std::cout <<"Test successful" << std::endl;
}

auto main() -> int {
    test_file();

    return 0;
}

