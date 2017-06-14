#include <fstream>
#include <iostream>
#include <string>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/Time.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.84";
static const uint64_t MILLION = 1000000;

/* This function simply copies a std::array into a new portion of memory. */
template <unsigned int SIZE>
std::pair<std::unique_ptr<char[]>, unsigned int> array_serializer_simple(
            const std::array<char, SIZE>& v) {
    std::unique_ptr<char[]> ptr(new char[sizeof(v)]);
    std::memcpy(ptr.get(), &v, sizeof(v));
    return std::make_pair(std::move(ptr), sizeof(v));
}

/* Takes a pointer to std::array passed in and returns as object. */
template <unsigned int SIZE>
std::array<char, SIZE> array_deserializer_simple(void* data,
            unsigned int /* size */) {
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
template <unsigned int SIZE>
void test_throughput(int numRuns) {
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, SIZE>>
        store(IP, PORT, array_serializer_simple<SIZE>,
                array_deserializer_simple<SIZE>);

    std::array<char, SIZE> array;

    // warm up
    std::cout << "Warming up" << std::endl;
    store.put(0, array);

    std::cout << "Warm up done" << std::endl;

    printf("size is %lu \n", sizeof(array));

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * numRuns; ++i) {
        store.put(0, array);
    }
    end = start.getUsElapsed();

    std::ofstream outfile;
    outfile.open("throughput_" + std::to_string(SIZE) + ".log");
    outfile << "throughput " + std::to_string(SIZE) + " test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(array)) / (end * 1.0 / MILLION)
            << std::endl;

    outfile.close();
}

auto main() -> int {
    test_throughput<128>(2000);
    test_throughput<4096>(2000);
    test_throughput<50 * 1024>(2000);
    test_throughput<1024 * 1024>(2000);
    test_throughput<10 * 1024 * 1024>(2000);
    test_throughput<100 * 1024 * 1024>(2000);
    return 0;
}
