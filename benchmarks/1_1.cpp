/* Copyright Joao Carreira 2016 */

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

#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 128;
static const uint64_t MILLION = 1000000;

struct Dummy {
    char data[SIZE];
    int id;
};


void test_sync() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy));

    cirrus::Stats stats;
    stats.reserve(MILLION);

    std::cout << "Measuring latencies.." << std::endl;
    for (uint64_t i = 0; i < MILLION; ++i) {
        cirrus::TimerFunction tf;
        store.put(d.get(), sizeof(Dummy), i % 1000, &mem);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * MILLION; ++i) {
        store.put(d.get(), sizeof(Dummy), i % 1000, &mem);

        if (i % 100000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("1_1.log");
    outfile << "1_1 test" << std::endl;
    outfile << "count: " << stats.getCount() << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "min: " << stats.min() << std::endl;
    outfile << "avg: " << stats.avg() << std::endl;
    outfile << "max: " << stats.max() << std::endl;
    outfile << "sd: " << stats.sd() << std::endl;
    outfile << "99%: " << stats.getPercentile(0.99) << std::endl;
    outfile.close();

    std::cout << "1_1 test" << std::endl;
    std::cout << "count: " << stats.getCount() << std::endl;
    std::cout << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    std::cout << "min: " << stats.min() << std::endl;
    std::cout << "avg: " << stats.avg() << std::endl;
    std::cout << "max: " << stats.max() << std::endl;
    std::cout << "sd: " << stats.sd() << std::endl;
    std::cout << "99%: " << stats.getPercentile(0.99) << std::endl;
}

auto main() -> int {
    // test burst of sync writes
    test_sync();

    return 0;
}

