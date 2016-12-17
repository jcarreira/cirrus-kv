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
    

struct Dummy {
    char data[SIZE];
    int id;
};

void test_file() {
    sirius::BladeFileClient client;
    client.connect(IP, PORT);

    sirius::FileAllocRec allocRec = client.allocate("/tmp/teste", SIZE);

    client.write_sync(allocRec, 0, 6, "TESTE");

    char input[10];
    client.read_sync(allocRec, 0, 6, input);

    if (strcmp(input, "TESTE") != 0)
        throw std::runtime_error("Test error");
}

auto main() -> int {
    test_file();

    return 0;
}

