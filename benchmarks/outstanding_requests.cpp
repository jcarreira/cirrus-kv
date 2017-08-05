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

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "client/TCPClient.h"

static const uint64_t MILLION = 1000000;
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";


/**
  * This benchmark tests the bandwidth of Cirrus when multiple operations
  * are outstanding. It tests how well Cirrus can take advantage of a full
  * pipeline of requests to keep the network hardware busy
  * @param outstanding How many outstanding requests are in flight at any time
  * @param write_size Size of each individual write
  */
void test_outstanding_requests(int outstanding, int write_size) {
    // how many current outstanding requests
    int cur_outstanding = 0;
}

auto main() -> int {
    test_outstanding_requests(10, 10);

    return 0;
}
