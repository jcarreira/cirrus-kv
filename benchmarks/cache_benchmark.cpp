#include <unistd.h>
#include <stdlib.h>
#include <cstdint>
#include <iostream>
#include <cctype>
#include <memory>
#include <string>
#include <chrono>
#include <fstream>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "cache_manager/CacheManager.h"
#include "client/TCPClient.h"
#include "cache_manager/LRAddedEvictionPolicy.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const unsigned int SIZE = 128;
const char IP[] = "10.10.49.83";
const int cache_size = 200;  // Arbitrary
const int MILLION = 1000000;

void print_stats(std::ostream& out, std::string test_name,
        uint64_t elapsed, uint64_t msg_sent) {
    out << test_name << " elapsed us: " << iterator_elapsed * MILLION
        << std::endl;
    out << test_name << " reads/s: "
        << (msg_sent * 1.0 / iterator_elapsed * MILLION) << std::endl;
}

/**
 * Finds the penalty that using the cache to get N items accrues over using
 * the store to access those N items. Each item is approx SIZE bytes.
 * @param num_items the number of items to put on the remote store,
 * which will then be iterated over.
 */
void test_cache(int num_items) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(cache_size);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, cache_size);

    // Insert the objects
    cirrus::Dummy<SIZE> d(300);
    for (int i = 0; i < num_items; i++) {
        cm.put(i, d);
    }

    cirrus::TimerFunction start;
    for (int i = 0; i < num_items; i++) {
        cirrus::Dummy<SIZE> val = cm.get(i);
    }
    uint64_t end = start.getUsElapsed();

    std::ofstream outfile;
    print_stats(outfile, "cache no prefetch", end, num_items);

    print_stats(std::cout, iterator_end, end, num_items);
}

void test_store(int num_items) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    // Insert the objects
    cirrus::Dummy<SIZE> d(300);
    for (int i = 0; i < num_items; i++) {
        store.put(i, d);
    }

    // Time with just the store
    cirrus::TimerFunction start;
    for (int i = 0; i < num_items; i++) {
        cirrus::Dummy<SIZE> val = store.get(i);
    }
    uint64_t end = start.getUsElapsed();

    print_stats(outfile, "store", end, num_items);
    print_stats(std::cout, "store", end, num_items);
}

// TODO(Tyler): Add additional benchmarks that time the time taken
// if the cache has ordered prefetching activated.
auto main() -> int {
    const int num_items = 1000;
    std::ofstream outfile;
    outfile.open("cache_benchmark.log");
    test_cache(outfile, num_items);
    outfile.close();
    return 0;
}
