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
#include <random>
#include <memory>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/Time.h"
#include "utils/Stats.h"

// TODO: Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 128;
static const uint64_t MILLION = 1000000;

/**
  * This benchmarks has two aims. The first aim is to find the distribution of
  * latencies for synchronous puts. To do this it times the time taken for
  * one million puts spread across 1000 object ids. The second aim is to
  * measure the throughput in terms of messages sent per second, which it
  * achieves by measuring the time needed for ten million puts.
  */
void test_sync() {
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT,
            cirrus::struct_serializer_simple<SIZE>,
            cirrus::struct_deserializer_simple<SIZE>);

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(i, d);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::Stats stats;
    stats.reserve(MILLION);

    std::cout << "Measuring latencies.." << std::endl;
    for (uint64_t i = 0; i < MILLION; ++i) {
        cirrus::TimerFunction tf;
        store.put(i % 1000, d);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * MILLION; ++i) {
        store.put(i % 1000, d);

        if (i % 100000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("1_1.log");
    outfile << "1_1 test" << std::endl;
    outfile << "count: " << stats.getCount() << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "min: " << stats.min() << std::endl;
    outfile << "avg: " << stats.avg() << std::endl;
    outfile << "max: " << stats.max() << std::endl;
    outfile << "sd: " << stats.sd() << std::endl;
    outfile << "99%: " << stats.getPercentile(0.99) << std::endl;
    outfile.close();

    std::cout << "1_1 test" << std::endl;
    std::cout << "count: " << stats.getCount() << std::endl;
    std::cout << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    std::cout << "min: " << stats.min() << std::endl;
    std::cout << "avg: " << stats.avg() << std::endl;
    std::cout << "max: " << stats.max() << std::endl;
    std::cout << "sd: " << stats.sd() << std::endl;
    std::cout << "99%: " << stats.getPercentile(0.99) << std::endl;
}

auto main() -> int {
    // test burst of sync writes
    test_sync();

    return 0;
}
