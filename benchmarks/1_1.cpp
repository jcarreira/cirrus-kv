/* Copyright Joao Carreira 2016 */

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
#include <unistd.h>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 128;

struct Dummy {
    char data[SIZE];
    int id;
};


void test_sync(int N) {
    sirius::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);
    sirius::Stats stats;

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < N; ++i) {
        store.put(d.get(), sizeof(Dummy), 1);
    }

    std::cout << "Warm up done" << std::endl;

    sirius::RDMAMem mem(d.get(), sizeof(Dummy));
    for (int i = 0; i < N; ++i) {
        sirius::TimerFunction tf;
        store.put(d.get(), sizeof(Dummy), 1, &mem);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    std::cout << "1_1 test" << std::endl;
    std::cout << "count: " << stats.getCount() << std::endl;
    std::cout << "min: " << stats.min() << std::endl;
    std::cout << "avg: " << stats.avg() << std::endl;
    std::cout << "max: " << stats.max() << std::endl;
    std::cout << "sd: " << stats.sd() << std::endl;
    std::cout << "99%: " << stats.getPercentile(0.99) << std::endl;
}

auto main() -> int {

    // test burst of 1000 sync writes
    test_sync(1000);

    return 0;
}

