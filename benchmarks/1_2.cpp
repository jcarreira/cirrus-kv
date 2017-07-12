#include <unistd.h>
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
#include "utils/Time.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 128;

/**
  * This benchmark measures the distribution of times necessary for
  * N asynchronous puts.
  */
void test_async(int N) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);
    cirrus::Stats stats;

    struct cirrus::Dummy<SIZE> d(42);
    cirrus::TimerFunction tfs[N];

    cirrus::ObjectStore<cirrus::Dummy<SIZE>>::ObjectStorePutFuture futures[N];
    std::cout << "Warming up." << std::endl;
    // warm up
    for (int i = 0; i < N; ++i) {
        store.put(i, d);
    }

    std::cout << "Warm up done" << std::endl;

    bool done[N];
    std::memset(done, 0, sizeof(done));
    int total_done = 0;

    for (int i = 0; i < N; ++i) {
        tfs[i].reset();
        futures[i] = store.put_async(i, d);
    }
    std::cout << "N puts started." << std::endl;
    while (total_done != N) {
        for (int i = 0; i < N; ++i) {
            if (!done[i]) {
                bool ret = futures[i].try_wait();
                if (ret) {
                    done[i] = true;
                    total_done++;
                    auto elapsed = tfs[i].getUsElapsed();
                    stats.add(elapsed);
                }
            }
        }
    }
    std::ofstream outfile;
    outfile.open("1_2.log");

    outfile << "count: " << stats.getCount() << std::endl;
    outfile << "min: " << stats.min() << std::endl;
    outfile << "avg: " << stats.avg() << std::endl;
    outfile << "max: " << stats.max() << std::endl;
    outfile << "sd: " << stats.sd() << std::endl;
    outfile << "99%: " << stats.getPercentile(0.99) << std::endl;
    outfile.close();

    std::cout << "count: " << stats.getCount() << std::endl;
    std::cout << "min: " << stats.min() << std::endl;
    std::cout << "avg: " << stats.avg() << std::endl;
    std::cout << "max: " << stats.max() << std::endl;
    std::cout << "sd: " << stats.sd() << std::endl;
    std::cout << "99%: " << stats.getPercentile(0.99) << std::endl;
}

auto main() -> int {
    std::cout << "Benchmark 1_2 started." << std::endl;
    // test burst of 100 async writes
    test_async(100);

    return 0;
}
