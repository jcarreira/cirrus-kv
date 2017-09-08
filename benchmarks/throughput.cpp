#include <fstream>
#include <iostream>
#include <string>
#include <iomanip>

#include "object_store/FullBladeObjectStore.h"
#include "utils/CirrusTime.h"
#include "client/TCPClient.h"
#include "tests/object_store/object_store_internal.h"
#include "cache_manager/CacheManager.h"
#include "cache_manager/LRAddedEvictionPolicy.h"
#include "iterator/CirrusIterable.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t MB = (1024 * 1024);
static const uint64_t GB = (1024 * MB);
const char PORT[] = "12345";
const char IP[] = "10.10.49.97";
static const uint64_t MILLION = 1000000;

/**
  * This benchmark tests the throughput of the system at various sizes
  * of message. When given a parameter SIZE and numRuns, it stores
  * numRuns objects of size SIZE under one objectID. The time to put the
  * objects is recorded, and statistics are computed.
  */
template <uint64_t SIZE>
void os_test_throughput(uint64_t numRuns, bool get = false) {
    cirrus::TCPClient client;
    cirrus::serializer_simple<std::array<char, SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, SIZE>>
        store(IP, PORT, &client,
                serializer,
                cirrus::deserializer_simple<std::array<char, SIZE>,
                    sizeof(std::array<char, SIZE>)>);

    std::string io_type = get ? "get" : "put";

    std::cout
        << "Test " << io_type << " store throughput with size: "
        << SIZE << std::endl;
    std::unique_ptr<std::array<char, SIZE>> array =
        std::make_unique<std::array<char, SIZE>>();

    // warm up
    store.put(0, *array);

    cirrus::TimerFunction start;
    for (uint64_t i = 0; i < numRuns; ++i) {
        if (get) {
            store.get(0);
        } else {
            store.put(0, *array);
        }
    }

    uint64_t end = start.getUsElapsed();

    std::ofstream outfile;
    std::string filename =
        "throughput_" + io_type + "_" + std::to_string(SIZE) + "_store.log";
    outfile.open(filename);
    outfile << io_type << " store throughput " + std::to_string(SIZE)
        << std::endl;
    outfile << std::fixed << "msg/s: " << numRuns / (end * 1.0 / MILLION)
        << std::endl;
    outfile << std::fixed << "MB/s: "
        << (numRuns * sizeof(*array)) / (end * 1.0 / MILLION) / MB
        << std::endl;

    outfile.close();
}

template <uint64_t SIZE>
void cm_test_throughput(uint64_t numRuns, bool get = false) {
    cirrus::TCPClient client;
    cirrus::serializer_simple<std::array<char, SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, SIZE>>
        store(IP, PORT, &client,
                serializer,
                cirrus::deserializer_simple<std::array<char, SIZE>,
                    sizeof(std::array<char, SIZE>)>);

    const int cache_size = 1000;;
    cirrus::LRAddedEvictionPolicy policy(cache_size);
    cirrus::CacheManager<std::array<char, SIZE>>
        cm(&store, &policy, cache_size);

    std::string io_type = get ? "get" : "put";

    std::cout
        << "Test " << io_type << " cache throughput with size: "
        << SIZE << std::endl;
    std::unique_ptr<std::array<char, SIZE>> array =
        std::make_unique<std::array<char, SIZE>>();

    // warm up
    store.put(0, *array);

    cirrus::TimerFunction start;
    for (uint64_t i = 0; i < numRuns; ++i) {
        if (get) {
            cm.get(0);
        } else {
            cm.put(0, *array);
        }
    }
    uint64_t end = start.getUsElapsed();

    std::ofstream outfile;
    std::string filename =
        "throughput_" + io_type + "_" + std::to_string(SIZE) + "_cache.log";
    outfile.open(filename);
    outfile << io_type << " cache throughput " + std::to_string(SIZE)
        << std::endl;
    outfile << std::fixed << "msg/s: " << i / (end * 1.0 / MILLION)
        << std::endl;
    outfile << std::fixed << "MB/s: "
        << (numRuns * sizeof(*array)) / (end * 1.0 / MILLION) / MB
        << std::endl;

    outfile.close();
}

template <uint64_t SIZE>
void iterator_throughput(uint64_t iterations, uint64_t read_ahead) {
    cirrus::TCPClient client;
    cirrus::serializer_simple<std::array<char, SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, SIZE>>
        store(IP, PORT, &client,
                serializer,
                cirrus::deserializer_simple<std::array<char, SIZE>,
                    sizeof(std::array<char, SIZE>)>);

    const int cache_size = 1000;;
    cirrus::LRAddedEvictionPolicy policy(cache_size);
    cirrus::CacheManager<std::array<char, SIZE>>
        cm(&store, &policy, cache_size);
    cirrus::CirrusIterable<std::array<char, SIZE>>
        iter(&cm, read_ahead, 0, iterations - 1);

    std::cout
        << "Test iterator throughput with size: "
        << SIZE << std::endl;
    std::unique_ptr<std::array<char, SIZE>> array =
        std::make_unique<std::array<char, SIZE>>();

    for (uint64_t i = 0; i < iterations; ++i)
        store.put(i, *array);

    cirrus::TimerFunction start;
    for (__attribute__((unused)) const auto& obj : iter) {
    }

    uint64_t end = start.getUsElapsed();

    std::ofstream outfile;
    std::string filename =
        "throughput_" + std::to_string(SIZE) + "_iterator.log";
    outfile.open(filename);
    outfile << "iterator throughput " + std::to_string(SIZE)
        << std::endl;
    outfile << std::fixed << "MB/s: "
        << (iterations * sizeof(*array)) / (end * 1.0 / MILLION) / MB
        << std::endl;

    outfile.close();
}

// Slightly larger object cause a segfault because objects live on the stack
auto main() -> int {
    uint64_t num_runs = 20000;
    std::cout << "Starting throughput benchmark of object store" << std::endl;
    os_test_throughput<128>(num_runs);                       // 128B
    os_test_throughput<4    * 1024>(num_runs);               // 4K
    os_test_throughput<50   * 1024>(num_runs);               // 50K
    os_test_throughput<1024 * 1024>(num_runs / 20);          // 1MB, total 1 gig

    os_test_throughput<128>(num_runs, true);                // 128B
    os_test_throughput<4    * 1024>(num_runs, true);        // 4K
    os_test_throughput<50   * 1024>(num_runs, true);        // 50K

    std::cout << "Starting throughput benchmark of cache manager" << std::endl;
    cm_test_throughput<128>(num_runs);                       // 128B
    cm_test_throughput<4    * 1024>(num_runs);               // 4K
    cm_test_throughput<50   * 1024>(num_runs);               // 50K
    cm_test_throughput<1024 * 1024>(num_runs / 20);          // 1MB, total 1 gig

    cm_test_throughput<128>(num_runs, true);                // 128B
    cm_test_throughput<4    * 1024>(num_runs, true);        // 4K
    cm_test_throughput<50   * 1024>(num_runs, true);        // 50K

    std::cout
        << "Starting throughput benchmark of cache manager with iterator"
        << std::endl;
    uint64_t iterations = 10000;
    uint64_t read_ahead = 10;
    iterator_throughput<128>(iterations, read_ahead);               // 128B
    iterator_throughput<4    * 1024>(iterations, read_ahead);       // 4K
    iterator_throughput<50   * 1024>(iterations, read_ahead);       // 50K
    iterator_throughput<1024 * 1024>(iterations / 20, read_ahead);  // 1MB
    return 0;
}

