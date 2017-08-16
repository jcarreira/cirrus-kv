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

#include "cache_manager/PrefetchPolicy.h"
#include "cache_manager/LRAddedEvictionPolicy.h"
#include "cache_manager/CacheManager.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint64_t MILLION = 1000000;
static const uint64_t N_ITER = MILLION;

void print_stats(std::ostream& out, const cirrus::Stats& stats,
        uint64_t size) {
    out << "Cache " << size << " byte benchmark" << std::endl;
    out << "count: " << stats.getCount() << std::endl;
    out << "min: " << stats.min() << " µs" << std::endl;
    out << "avg: " << stats.avg() << " µs" << std::endl;
    out << "max: " << stats.max() << " µs"<< std::endl;
    out << "sd: " << stats.sd() << " µs" << std::endl;
    out << "99%: " << stats.getPercentile(0.99) << " µs" << std::endl;
    out << std::endl;
}

/**
  * This benchmark has aims to find the latency when getting items from a
  * cache of one million objects.
  */
template <uint64_t SIZE>
void test_get_latency(std::ofstream& outfile) {
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


    cirrus::LRAddedEvictionPolicy policy(N_ITER);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, N_ITER);

    // populate the cache. We may want to change this to prefetch
    for (uint64_t i = 0; i < N_ITER; i++) {
        cm.get(i);
    }

    cirrus::Stats stats;
    stats.reserve(N_ITER);

    std::cout << "Measuring latencies.." << std::endl;
    for (uint64_t i = 0; i < N_ITER; ++i) {
        cirrus::TimerFunction tf;
        cm.get(i);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    print_stats(outfile, stats, SIZE);

    print_stats(std::cout, stats, SIZE);
}

auto main() -> int {
    // test burst of sync writes
    std::ofstream outfile;
    outfile.open("cache_latency.log");
    test_get_latency<32>(outfile);
    test_get_latency<1024>(outfile);
    test_get_latency<4 * 1024>(outfile);
    // test_get_latency<32 * 1024>(outfile);
    // test_get_latency<1024 * 1024>(outfile);
    outfile.close();

    return 0;
}
