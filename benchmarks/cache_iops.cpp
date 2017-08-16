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

#include "cache_manager/LRAddedEvictionPolicy.h"
#include "cache_manager/CacheManager.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t KB = 1024;
// Size of items being fetched
static const uint64_t SIZE = 4 * KB;
// Capacity of the cache
static const uint64_t CACHE_SIZE = 500;
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint64_t MILLION = 1000000;
// Number of times items will be fetched from the store/ cache
static const uint64_t N_ITER = 10000;
// Number of items that exist
static const uint64_t N_ITEMS = 1000;

void print_stats(std::ostream& out, uint64_t msg_sent, uint64_t elapsed_us,
    uint64_t size, bool is_cache) {
    if (is_cache) {
        out << "Results with caching" << std::endl;
    } else {
        out << "Results without caching" << std::endl;
    }
    out << "IO/s: " << (msg_sent * 1.0 / elapsed_us * MILLION)
        << std::endl;
    out << "bytes/s: " << (size * msg_sent * 1.0
        / elapsed_us * MILLION) << std::endl;
    out << std::endl;
}

/**
  * This benchmark has two aims
  * 1. find the distribution of latencies for synchronous puts
  * 2. measure the throughput in terms of messages sent / second
  */
void test_iops(std::ofstream& outfile) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(CACHE_SIZE);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, CACHE_SIZE);


    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (uint64_t i = 0; i < N_ITEMS; ++i) {
        store.put(i, d);
    }

    // Populate the vector if items to fetch
    std::vector<uint64_t> to_fetch(N_ITER);

    // Modify these parameters to change the distribution
    int mean = N_ITEMS/2;
    int std_dev = N_ITEMS/16;
    std::normal_distribution<> dist(mean, std_dev);

    std::random_device rd;
    std::mt19937 gen(rd());

    for (uint64_t i = 0; i < N_ITER; i++) {
        to_fetch[i] = std::round(dist(gen));
    }

    std::cout << "Warm up done" << std::endl;

    std::cout << "Measuring msgs/s.. w/ cache" << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;

    for (; i < N_ITER; i++) {
        cm.get(to_fetch[i]);
    }

    uint64_t elapsed_us = start.getUsElapsed();


    print_stats(outfile, i, elapsed_us, SIZE, true);

    print_stats(std::cout, i, elapsed_us, SIZE, true);


    std::cout << "Measuring msgs/s.. w/o cache" << std::endl;

    i = 0;
    cirrus::TimerFunction store_start;

    for (; i < N_ITER; i++) {
        store.get(to_fetch[i]);
    }

    uint64_t elapsed_us_store = store_start.getUsElapsed();
    print_stats(outfile, i, elapsed_us_store, SIZE, false);

    print_stats(std::cout, i, elapsed_us_store, SIZE, false);
}


auto main() -> int {
    // test burst of sync writes
    std::ofstream outfile;
    outfile.open("cache_iops.log");
    test_iops(outfile);
    outfile.close();

    return 0;
}
