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

#include "src/client/BladeClient.h"
#include "src/utils/Time.h"

// TODO: Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
static const uint64_t MB = (1024*1024);
static const int N_THREADS = 2;
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1024;

void test_fetchadd() {
    cirrus::BladeClient client;

    client.connect(IP, PORT);

    cirrus::AllocationRecord allocRec = client.allocate(SIZE);

    client.fetchadd_sync(allocRec, 0, 42);

    client.deallocate(allocRec);
}

auto main() -> int {
    test_fetchadd();

    return 0;
}
