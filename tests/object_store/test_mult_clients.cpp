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
static const int N_THREADS = 20;
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1024;

struct Dummy {
    char data[SIZE];
    int id;
};

/* This function simply copies a struct Dummy into a new portion of memory. */
std::pair<void*, unsigned int> struct_serializer_simple(const struct Dummy& v) {
    void *ptr = malloc(sizeof(struct Dummy));
    std::memcpy(ptr, &v, sizeof(struct Dummy));
    return std::make_pair(ptr, sizeof(struct Dummy));
}

/* Takes a pointer to struct Dummy passed in and returns as object. */
struct Dummy struct_deserializer_simple(void* data, unsigned int /* size */) {
    struct Dummy *ptr = (struct Dummy *) data;
    struct Dummy retDummy;
    retDummy.id = ptr->id;
    std::memcpy(&retDummy.data, &(ptr->data), SIZE);
    return retDummy;
}


uint64_t total_puts = 0;

void test_multiple_clients() {
    cirrus::TimerFunction tf("connect time", true);

    std::thread* threads[N_THREADS];

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([dis, gen]() {
          cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT,
                       struct_serializer_simple, struct_deserializer_simple);

          for (int i = 0; i < 100; ++i) {
                struct Dummy d;
                int rnd = std::rand();
                d.id = rnd;

                store.put(1, d);

                struct Dummy d2 = store.get(1);

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
