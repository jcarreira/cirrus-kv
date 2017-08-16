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

#include "cache_manager/CacheManager.h"
#include "iterator/CirrusIterable.h"
#include "cache_manager/LRAddedEvictionPolicy.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint64_t MILLION = 1000000;
static const uint64_t N_ITER = MILLION;


/**
 * Does a sort of word count by splitting a string into words,
 * then counting occurences of the words.
 */
void process(const std::string& s, std::map<std::string, uint64_t> *counts) {
    std::istringstream iss(s);
    std::vector<std::string> words{std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}};
    for (const auto& word : words) {
        ++(counts->operator[](word));
    }
}

/**
 * Reads in the full works of mark twain seven times and parses them into
 * std strings (~110 MB). Once these strings reach 4kB, they are pushed
 * to the remote store.
 * @return The highest objectid placed on the server, beginning from 0.
 */
uint64_t setup() {
    cirrus::TCPClient client;

    cirrus::ostore::FullBladeObjectStoreTempl<std::string>
        store(IP, PORT, &client,
            cirrus::string_serializer_simple,
            cirrus::string_deserializer_simple);

    std::ifstream file("benchmarks/complete-works-mark-twain.txt");
    std::string str;
    std::string full_string;
    uint64_t i = 0;
    while (std::getline(file, str)) {
        full_string.append(str);
        if (full_string.size() >= 4 * 1024) {
            store.put(i++, full_string);
            full_string.clear();
        }
    }
    file.close();
    return i - 1;
}

void print_stats(std::ostream& out, uint64_t elapsed_us, uint64_t msg_sent,
    bool is_iterator) {
    if (is_iterator) {
        out << "Results with iterator" << std::endl;
    } else {
        out << "Results without iterator" << std::endl;
    }

    out << "µs elapsed: " << elapsed_us << std::endl;
    out << "msg/s: " << (msg_sent * 1.0 / elapsed_us * MILLION)
        << std::endl;
    out << std::endl;
}

/**
  * This benchmark iterates through each item in the store and counts
  * the words in the strings it receives. It does not use a cirrus::Iterator
  * @param outfile the ofstream to write to
  * @param highest_id the highest continuous id in the remote store
  */
void test_iteration_store(std::ofstream& outfile, uint64_t highest_id) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<std::string>
        store(IP, PORT, &client,
            cirrus::string_serializer_simple,
            cirrus::string_deserializer_simple);


    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    std::map<std::string, uint64_t> counts;
    cirrus::TimerFunction start;

    for (; i <= highest_id; i++) {
        std::string str = store.get(i);
        process(str, &counts);
    }

    uint64_t elapsed_us = start.getUsElapsed();

    print_stats(outfile, elapsed_us, i, false);

    print_stats(std::cout, elapsed_us, i, false);
}

/**
  * This benchmark iterates through each item in the store and counts
  * the words in the strings it receives. It makes use of a cirrus::Iterator
  * @param outfile the ofstream to write to
  * @param highest_id the highest continuous id in the remote store
  */
void test_iteration_cache(std::ofstream& outfile, uint64_t highest_id) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<std::string>
        store(IP, PORT, &client,
            cirrus::string_serializer_simple,
            cirrus::string_deserializer_simple);

    cirrus::LRAddedEvictionPolicy policy(100);
    cirrus::CacheManager<std::string> cm(&store, &policy, 100);

    // Use iterator to retrieve
    uint64_t read_ahead = 10;
    cirrus::CirrusIterable<std::string> iter(&cm, read_ahead, 0, highest_id);

    std::cout << "Measuring msgs/s.." << std::endl;
    std::map<std::string, uint64_t> counts;
    cirrus::TimerFunction start;

    for (const auto& str : iter) {
        process(str, &counts);
    }

    uint64_t elapsed_us = start.getUsElapsed();

    print_stats(outfile, elapsed_us, highest_id, true);

    print_stats(std::cout, elapsed_us, highest_id, true);
}

auto main() -> int {
    // test burst of sync writes
    std::ofstream outfile;
    outfile.open("iterator_performance.log");
    uint64_t highest_id = setup();
    std::cout << highest_id << std::endl;
    test_iteration_store(outfile, highest_id);
    test_iteration_cache(outfile, highest_id);
    outfile.close();

    return 0;
}
