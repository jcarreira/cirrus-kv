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

static const uint64_t MILLION = 1000000;
static const int N_THREADS = 10;
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 128;
static const uint64_t N_MSG = 1000000;

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
uint64_t total_time = 0;

std::atomic<int> count;

void test_multiple_clients() {
    std::thread* threads[N_THREADS];

    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([]() {
            cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT,
			    struct_serializer_simple,
			    struct_deserializer_simple);

            struct Dummy d;
            d.id = 42;
            // warm up
            cirrus::RDMAMem mem(&d, sizeof(Dummy));
            for (uint64_t i = 0; i < 100; ++i) {
                store.put(i, d, &mem);
            }

            // barrier
            count++;
            while (count != N_THREADS) {}

            cirrus::TimerFunction tf;
            for (uint64_t i = 0; i < N_MSG; ++i) {
                store.put(i % 100, d, &mem);
            }

            total_time += tf.getUsElapsed();
        });
    }

    for (int i = 0; i < N_THREADS; ++i)
        threads[i]->join();

    std::cout << "Total time: " << total_time << std::endl;
    std::cout << "Average (us) per msg: "
        << total_time / N_THREADS / N_MSG << std::endl;
    std::cout << "MSG/s: "
        << N_MSG * N_THREADS / (total_time * 1.0 / MILLION) << std::endl;

    std::ofstream outfile;
    outfile.open("1_3.log");

    outfile << "Total time: " << total_time << std::endl;
    outfile << "Average (us) per msg: "
        << total_time / N_THREADS / N_MSG << std::endl;
    outfile << "MSG/s: "
        << N_MSG * N_THREADS / (total_time * 1.0 / MILLION) << std::endl;
    outfile.close();
}

auto main() -> int {
    test_multiple_clients();

    return 0;
}

