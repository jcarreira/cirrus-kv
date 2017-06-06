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
static const uint32_t SIZE = 1024*1024;
static const uint64_t MILLION = 1000000;

struct Dummy {
    char data[SIZE];
    int id;
};


void test_exhaustion() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    // warm up
    std::cout << "Putting 1000" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy), i);
    }

    std::cout << "Done putting 1000" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy));

    std::cout << "Putting one million objects" << std::endl;
    for (uint64_t i = 0; i < MILLION; ++i) {
        store.put(d.get(), sizeof(Dummy), i, &mem);
    }
}

auto main() -> int {
    test_exhaustion();
    printf("should have been unreachable");
    return -1;
}
