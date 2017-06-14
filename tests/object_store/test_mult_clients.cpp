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
static const int N_THREADS = 20;
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1024;

struct Dummy {
    char data[SIZE];
    int id;
};

uint64_t total_puts = 0;

void test_multiple_clients() {
    cirrus::TimerFunction tf("connect time", true);

    std::thread* threads[N_THREADS];

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([dis, gen]() {
            cirrus::ostore::FullBladeObjectStoreTempl<> store(IP, PORT);
            for (int i = 0; i < 100; ++i) {
                std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
                int rnd = std::rand();
                d->id = rnd;

                store.put(d.get(), sizeof(Dummy), 1);
                Dummy* d2 = new Dummy;

                store.get(1, d2);

                if (d2->id != rnd)
                    throw std::runtime_error("mismatch");

                total_puts++;
            }
        });
    }

    for (int i = 0; i < N_THREADS; ++i)
        threads[i]->join();

    std::cout << "Total puts: " << total_puts << std::endl;
}

auto main() -> int {
    test_multiple_clients();

    return 0;
}

