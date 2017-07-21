#include <fstream>
#include <iostream>
#include <string>

#include "object_store/FullBladeObjectStore.h"
#include "utils/CirrusTime.h"
#include "client/TCPClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint64_t MILLION = 1000000;

/* This function simply copies a std::array into a new portion of memory. */
template <uint64_t SIZE>
std::pair<std::unique_ptr<char[]>, uint64_t> array_serializer_simple(
            const std::array<char, SIZE>& v) {
    std::unique_ptr<char[]> ptr(new char[sizeof(v)]);
    std::memcpy(ptr.get(), &v, sizeof(v));
    return std::make_pair(std::move(ptr), sizeof(v));
}

/* Takes a pointer to std::array passed in and returns as object. */
template <uint64_t SIZE>
std::array<char, SIZE> array_deserializer_simple(void* data,
            uint64_t /* size */) {
    std::array<char, SIZE> *ptr = (std::array<char, SIZE> *) data;
    std::array<char, SIZE> retArray;
    retArray = *ptr;
    return retArray;
}

/**
  * This benchmark tests the throughput of the system at various sizes
  * of message. When given a parameter SIZE and numRuns, it stores
  * numRuns objects of size SIZE under one objectID. The time to put the
  * objects is recorded, and statistics are computed.
  */
template <uint64_t SIZE>
void test_throughput(int numRuns) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, SIZE>>
        store(IP, PORT, &client, array_serializer_simple<SIZE>,
                array_deserializer_simple<SIZE>);

    std::cout << "Creating the array to put." << std::endl;
    std::unique_ptr<std::array<char, SIZE>> array =
        std::make_unique<std::array<char, SIZE>>();

    // warm up
    std::cout << "Warming up" << std::endl;
    store.put(0, *array);
    std::cout << "Warm up done" << std::endl;

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < numRuns; ++i) {
        store.put(0, *array);
    }
    end = start.getUsElapsed();

    std::ofstream outfile;
    outfile.open("throughput_" + std::to_string(SIZE) + ".log");
    outfile << "throughput " + std::to_string(SIZE) + " test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(*array)) / (end * 1.0 / MILLION)
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
void test_throughput_get(int numRuns) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, SIZE>>
        store(IP, PORT, &client, array_serializer_simple<SIZE>,
                array_deserializer_simple<SIZE>);

    std::cout << "Creating the array to put." << std::endl;
    std::unique_ptr<std::array<char, SIZE>> array =
        std::make_unique<std::array<char, SIZE>>();

    // warm up
    std::cout << "Setting up" << std::endl;
    store.put(0, *array);
    std::cout << "Setup done" << std::endl;

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < numRuns; ++i) {
        store.get(0);
    }
    end = start.getUsElapsed();

    std::ofstream outfile;
    outfile.open("throughput_get_" + std::to_string(SIZE) + ".log");
    outfile << "throughput " + std::to_string(SIZE) + " test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(*array)) / (end * 1.0 / MILLION)
            << std::endl;

    outfile.close();
}

auto main() -> int {
    uint64_t num_runs = 20000;

    test_throughput<128>(num_runs);                // 128B
    test_throughput<4    * 1024>(num_runs);        // 4K
    test_throughput<50   * 1024>(num_runs);        // 50K
    test_throughput<1024 * 1024>(num_runs / 20);        // 1MB, total 1 gig
    test_throughput<10   * 1024 * 1024>(num_runs / 100);  // 10MB, total 2 gig
    test_throughput<50 * 1024 * 1024>(num_runs / 100);
    test_throughput<100  * 1024 * 1024>(50);  // 100MB, total 5 gig
    
    test_throughput_get<128>(num_runs);                // 128B
    test_throughput_get<4    * 1024>(num_runs);        // 4K
    test_throughput_get<50   * 1024>(num_runs);        // 50K
    // Objects larger than this cause a segfault, likely as the store does not
    // return by reference, and the object is placed on the stack
    test_throughput_get<1024 * 1024>(num_runs / 20);        // 1MB, total 1 gig

    return 0;
}
