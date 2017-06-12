/* Copyright Joao Carreira 2016 */

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
template <unsigned int size>
std::pair<void*, unsigned int> array_serializer_simple(
            const std::array<char, size>& v) {
    void *ptr = malloc(sizeof(v));
    std::memcpy(ptr, &v, sizeof(v));
    return std::make_pair(ptr, sizeof(v));
}

/* Takes a pointer to std::array passed in and returns as object. */
template <unsigned int size>
std::array<char, size> array_deserializer_simple(void* data,
            unsigned int /* size */) {
    std::array<char, size> *ptr = (std::array<char, size> *) data;
    std::array<char, size> retArray;
    retArray = *ptr;
    return retArray;
}


template <unsigned int size>
void test_throughput(int numRuns) {
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, size>>
        store(IP, PORT, array_serializer_simple<size>,
                array_deserializer_simple<size>);

    std::array<char, size> array;

    // warm up
    std::cout << "Warming up" << std::endl;
    store.put(0, array);

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(&array, sizeof(array));
    printf("size is %lu \n", sizeof(array));

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * numRuns; ++i) {
        store.put(0, array, &mem);
    }
    end = start.getUsElapsed();

    std::ofstream outfile;
    outfile.open("throughput_" + std::to_string(size) + ".log");
    outfile << "throughput " + std::to_string(size) + " test" << std::endl;
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
