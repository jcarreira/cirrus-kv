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
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint32_t SIZE = 128;
static const uint64_t MILLION = 1000000;
static const uint64_t N_ITER = 100;

void print_stats(std::ostream& out, const cirrus::Stats& stats,
        uint64_t msg_sent, uint64_t elapsed_us) {
    out << "1_1 test" << std::endl;
    out << "count: " << stats.getCount() << std::endl;
    out << "msg/s: " << (msg_sent * 1.0 / elapsed_us * MILLION)
        << std::endl;
    out << "min: " << stats.min() << std::endl;
    out << "avg: " << stats.avg() << std::endl;
    out << "max: " << stats.max() << std::endl;
    out << "sd: " << stats.sd() << std::endl;
    out << "99%: " << stats.getPercentile(0.99) << std::endl;
}

/**
  * This benchmarks has two aims. The first aim is to find the distribution of
  * latencies for synchronous puts. To do this it times the time taken for
  * one million puts spread across 1000 object ids. The second aim is to
  * measure the throughput in terms of messages sent per second, which it
  * achieves by measuring the time needed for ten million puts.
  */
void test_sync() {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>, SIZE>);

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < N_ITER; ++i) {
        store.put(i, d);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::Stats stats;
    stats.reserve(N_ITER);

    std::cout << "Measuring latencies.." << std::endl;
    for (uint64_t i = 0; i < N_ITER; ++i) {
        cirrus::TimerFunction tf;
        store.put(i % 1000, d);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;

    for (; i < N_ITER; ++i) {
        store.put(i % 10, d);
    }

    uint64_t elapsed_us = start.getUsElapsed();

    std::ofstream outfile;
    outfile.open("1_1.log");
    print_stats(outfile, stats, i, elapsed_us);
    outfile.close();

    print_stats(std::cout, stats, i, elapsed_us);
}

auto main() -> int {
    // test burst of sync writes
    test_sync();

    return 0;
}
