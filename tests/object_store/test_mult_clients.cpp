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
#include "src/object_store/object_store_internal.h"
#include "src/utils/Time.h"

static const uint64_t GB = (1024*1024*1024);
static const uint64_t MB = (1024*1024);
static const int N_THREADS = 20;
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1024;

uint64_t total_puts = 0;

/**
  * Checks that the system works properly when multiple clients get and put.
  * These clients each use their own instance of the store.
  */
void test_multiple_clients() {
    cirrus::TimerFunction tf("connect time", true);

    std::thread* threads[N_THREADS];

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([dis, gen]() {
          cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
              store(IP, PORT,
                       cirrus::struct_serializer_simple<SIZE>,
                       cirrus::struct_deserializer_simple<SIZE>);

          for (int i = 0; i < 100; ++i) {
                int rnd = std::rand();

                struct cirrus::Dummy<SIZE> d(rnd);

                store.put(1, d);

                struct cirrus::Dummy<SIZE> d2 = store.get(1);

                if (d2.id != rnd)
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
