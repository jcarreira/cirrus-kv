/* Copyright Joao Carreira 2016 */

#include <fstream>
#include <iostream>
#include <string>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.84";
static const uint64_t MILLION = 1000000;

template <int size>
void test_throughput(int numRuns) {
    cirrus::ostore::FullBladeObjectStoreTempl<std::array<char, size>> store(IP, PORT);

    std::array<char, size> *ptr = new std::array<char, size>;

    // warm up
    std::cout << "Warming up" << std::endl;
    store.put(ptr, sizeof(*ptr), 0);

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(ptr, sizeof(*ptr));
    printf("size is %lu \n", sizeof(*ptr));
    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * numRuns; ++i) {
        store.put(ptr, sizeof(*ptr), 0, &mem);
    }
    end = start.getUsElapsed();

    std::ofstream outfile;
    outfile.open("throughput_" + std::to_string(size) + ".log");
    outfile << "throughput " + std::to_string(size) + " test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(*ptr)) / (end * 1.0 / MILLION)  << std::endl;

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
