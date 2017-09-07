#include <fstream>
#include <iostream>
#include <string>

#include "object_store/FullBladeObjectStore.h"
#include "utils/CirrusTime.h"
#include "client/TCPClient.h"
#include "tests/object_store/object_store_internal.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t MB = (1024 * 1024);
static const uint64_t GB = (1024 * MB);
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint64_t MILLION = 1000000;

/**
  * This benchmark tests the throughput of the system at various sizes
  * of message. When given a parameter SIZE and numRuns, it stores
  * numRuns objects of size SIZE under one objectID. The time to put the
  * objects is recorded, and statistics are computed.
  */
template <uint64_t SIZE>
void os_test_throughput(uint64_t numRuns) {
    cirrus::TCPClient client;
    cirrus::serializer_simple<std::array<char, SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, SIZE>>
        store(IP, PORT, &client,
                serializer,
                cirrus::deserializer_simple<std::array<char, SIZE>,
                    sizeof(std::array<char, SIZE>)>);

    std::cout << "Test put throughput with size: " << SIZE << std::endl;
    std::unique_ptr<std::array<char, SIZE>> array =
        std::make_unique<std::array<char, SIZE>>();

    // warm up
    store.put(0, *array);

    uint64_t end;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < numRuns; ++i) {
        store.put(0, *array);
    }
    end = start.getUsElapsed();

    std::ofstream outfile;
    outfile.open("throughput_" + std::to_string(SIZE) + ".log");
    outfile << "put throughput " + std::to_string(SIZE) + " test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "MB/s: " << (numRuns * sizeof(*array)) / (end * 1.0 / MILLION) / MB
            << std::endl;

    outfile.close();
}

/**
  * This benchmark tests the throughput of the system at various sizes
  * of message. When given a parameter SIZE and numRuns, it retrieves
  * numRuns objects of size SIZE from one objectID. The time to get the
  * objects is recorded, and statistics are computed.
  */
template <uint64_t SIZE>
void os_test_throughput_get(uint64_t numRuns) {
    cirrus::TCPClient client;
    cirrus::serializer_simple<std::array<char, SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, SIZE>>
        store(IP, PORT, &client,
                serializer,
                cirrus::deserializer_simple<std::array<char, SIZE>,
                    sizeof(std::array<char, SIZE>)>);

    std::cout << "Test get throughput with size: " << SIZE << std::endl;
    std::unique_ptr<std::array<char, SIZE>> array =
        std::make_unique<std::array<char, SIZE>>();

    // warm up
    store.put(0, *array);

    uint64_t end;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < numRuns; ++i) {
        store.get(0);
    }
    end = start.getUsElapsed();

    std::ofstream outfile;
    outfile.open("throughput_get_" + std::to_string(SIZE) + ".log");
    outfile << "get throughput " + std::to_string(SIZE) + " test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "MB/s: " << (i * sizeof(*array)) / (end * 1.0 / MILLION) / MB
            << std::endl;

    outfile.close();
}

auto main() -> int {
    uint64_t num_runs = 20000;
    std::cout << "Starting throughput benchmark of object store" << std::endl;
    os_test_throughput<128>(num_runs);                       // 128B
    os_test_throughput<4    * 1024>(num_runs);               // 4K
    os_test_throughput<50   * 1024>(num_runs);               // 50K
    os_test_throughput<1024 * 1024>(num_runs / 20);          // 1MB, total 1 gig
    os_test_throughput<10   * 1024 * 1024>(num_runs / 100);  // 10MB, total 2 gig
    os_test_throughput<50   * 1024 * 1024>(num_runs / 100);  // 50MB
    os_test_throughput<100  * 1024 * 1024>(50);              // 100MB, total 5 gig

    os_test_throughput_get<128>(num_runs);                // 128B
    os_test_throughput_get<4    * 1024>(num_runs);        // 4K
    os_test_throughput_get<50   * 1024>(num_runs);        // 50K
    // Objects larger than this cause a segfault, likely as the store does not
    // return by reference, and the object is placed on the stack
    // os_test_throughput_get<1024 * 1024>(num_runs / 20);        // 1MB, total 1 gig
    
    std::cout << "Starting throughput benchmark of cache manager" << std::endl;
    return 0;
}
