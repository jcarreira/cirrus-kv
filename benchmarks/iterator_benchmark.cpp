#include <unistd.h>
#include <stdlib.h>
#include <cstdint>
#include <iostream>
#include <cctype>
#include <memory>
#include <chrono>
#include <fstream>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "cache_manager/CacheManager.h"
#include "iterator/CirrusIterable.h"
#include "client/TCPClient.h"
#include "cache_manager/LRAddedEvictionPolicy.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const unsigned int SIZE = 128;
const char IP[] = "127.0.0.1";
const int cache_size = 200;  // Arbitrary
const int read_ahead = 20;  // Arbitrary
const int MILLION = 1000000;

/**
 * Prints the stats from a test to the given ostream.
 * @param out the ostream to write the results to
 * @param iterator_elapsed time elapsed during the iterator phase of the test
 * @param regular_elapsed time elapsed during the regular phase of the test
 * @parm msgs_sent the number of messages sent during the test
 */
void print_stats(std::ostream& out, uint64_t iterator_elapsed,
        uint64_t regular_elapsed, uint64_t msgs_sent) {
    out << "Iterator Throughput Benchmark" << std::endl;
    out << "Iterator elapsed us: " << iterator_elapsed * MILLION << std::endl;
    out << "Regular elapsed us: " << regular_elapsed * MILLION << std::endl;
    out << "iterator reads/s: "
        << (msgs_sent * 1.0 / iterator_elapsed * MILLION)
        << std::endl;
    out << "Regular reads/s: " << (msgs_sent * 1.0 / regular_elapsed * MILLION)
        << std::endl;
}

/**
 * Compares the time to retrieve N items using the iterator vs
 * fetching each individually.
 * @param num_items the number of items to put on the remote store,
 * which will then be iterated over.
 */
void test_iterator(int num_items) {
    cirrus::TCPClient<cirrus::Dummy<SIZE>> client;
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            &client,
            serializer,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(cache_size);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, cache_size);

    // Insert the objects
    cirrus::Dummy<SIZE> d(300);
    for (int i = 0; i < num_items; i++) {
        cm.put(i, d);
    }

    // Test time to access all with an iterator.
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, read_ahead, 0,
        num_items - 1);

    cirrus::TimerFunction iterator_start;
    for (auto it = iter.begin(); it != iter.end(); it++) {
        cirrus::Dummy<SIZE> val = *it;
    }
    uint64_t iterator_end = iterator_start.getUsElapsed();

    cirrus::LRAddedEvictionPolicy policy2(cache_size);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm2(&store, &policy2, cache_size);

    // Time without the iterator
    cirrus::TimerFunction regular_start;
    for (int i = 0; i < num_items; i++) {
        cirrus::Dummy<SIZE> val = cm.get(i);
    }
    uint64_t regular_end = regular_start.getUsElapsed();

    std::ofstream outfile;
    outfile.open("throughput_benchmark.log");
    print_stats(outfile, iterator_end, regular_end, num_items);
    outfile.close();

    print_stats(std::cout, iterator_end, regular_end, num_items);
}

auto main() -> int {
    const int num_items = 1000;
    test_iterator(num_items);
    return 0;
}
