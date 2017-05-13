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
#include <memory>
#include <random>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/Time.h"

static const uint64_t GB = (1024*1024*1024);
static const uint64_t MB = (1024*1024);
static const int N_THREADS = 2;
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1024;
    
cirrus::ostore::FullBladeObjectStoreTempl<> store(IP, PORT);

struct Dummy {
    char data[SIZE];
    int id;
};

void test_mt() {
    cirrus::TimerFunction tf("connect time", true);

    std::thread* threads[N_THREADS];

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([dis, gen]() {
            for (int i = 0; i < 100; ++i) {
                std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
                int rnd = std::rand();
                d->id = rnd;

                store.put(d.get(), sizeof(Dummy), 1);
                Dummy* d2 = new Dummy;

                store.get(1, d2);

                if (d2->id != rnd)
                    throw std::runtime_error("mismatch");
            }
        });
    }

    for (int i = 0; i < N_THREADS; ++i)
        threads[i]->join();
}

auto main() -> int {
    test_mt();

    return 0;
}

