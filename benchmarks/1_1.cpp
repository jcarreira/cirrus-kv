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
static const uint64_t MILLION = 1000000;
static const uint64_t N_ITER = 10000;

void print_stats(std::ostream& out, const cirrus::Stats& stats,
        uint64_t msg_sent, uint64_t elapsed_us, uint64_t size, bool is_put) {
    out << size << " byte";
    if (is_put) {
        out << " put benchmark" << std::endl;
    } else {
        out << " get benchmark" << std::endl;
    }
    out << "count: " << stats.getCount() << std::endl;
    out << "msg/s: " << (msg_sent * 1.0 / elapsed_us * MILLION)
        << std::endl;
    out << "bytes/s: " << (size * msg_sent * 1.0
        / elapsed_us * MILLION) << std::endl;
    out << "min: " << stats.min() << " µs" << std::endl;
    out << "avg: " << stats.avg() << " µs" << std::endl;
    out << "max: " << stats.max() << " µs"<< std::endl;
    out << "sd: " << stats.sd() << " µs" << std::endl;
    out << "99%: " << stats.getPercentile(0.99) << " µs" << std::endl;
    out << std::endl;
}

/**
  * This benchmark has two aims
  * 1. find the distribution of latencies for synchronous puts
  * 2. measure the throughput in terms of messages sent / second
  */
template <uint64_t SIZE>
void test_put_sync(std::ofstream& outfile) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (uint64_t i = 0; i < N_ITER; ++i) {
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


    print_stats(outfile, stats, i, elapsed_us, SIZE, true);

    print_stats(std::cout, stats, i, elapsed_us, SIZE, true);
}

/**
  * This benchmark has two aims
  * 1. find the distribution of latencies for synchronous puts
  * 2. measure the throughput in terms of messages sent / second
  */
template <uint64_t SIZE>
void test_get_sync(std::ofstream& outfile) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (uint64_t i = 0; i < N_ITER; ++i) {
        store.put(i, d);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::Stats stats;
    stats.reserve(N_ITER);

    std::cout << "Measuring latencies.." << std::endl;
    for (uint64_t i = 0; i < N_ITER; ++i) {
        cirrus::TimerFunction tf;
        store.get(i % 1000);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;

    for (; i < N_ITER; ++i) {
        store.get(i % 10);
    }

    uint64_t elapsed_us = start.getUsElapsed();


    print_stats(outfile, stats, i, elapsed_us, SIZE, false);

    print_stats(std::cout, stats, i, elapsed_us, SIZE, false);
}

auto main() -> int {
    // test burst of sync writes
    std::ofstream outfile;
    outfile.open("store_latency.log");
    test_put_sync<32>(outfile);
    test_put_sync<1024>(outfile);
    test_put_sync<4 * 1024>(outfile);
    test_put_sync<32 * 1024>(outfile);
    test_put_sync<1024 * 1024>(outfile);
    test_get_sync<32>(outfile);
    test_get_sync<1024>(outfile);
    test_get_sync<4 * 1024>(outfile);
    test_get_sync<32 * 1024>(outfile);
    test_get_sync<1024 * 1024>(outfile);
    outfile.close();

    return 0;
}
