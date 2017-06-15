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

// TODO: Remove hardcoded IP and PORT
static const uint64_t MILLION = 1000000;
static const int N_THREADS = 10;
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 128;
static const uint64_t N_MSG = 1000000;

uint64_t total_puts = 0;
uint64_t total_time = 0;

std::atomic<int> count;

/**
  * This benchmark tests the performance of the system when multiple clients
  * are connected to the same store. To do so, it creates ten new threads,
  * each of which connects to the store and sends N_MSG puts spread across
  * 100 different object ids. The time taken for these puts is then recorded
  * and statistics computed.
  */
void test_multiple_clients() {
    std::thread* threads[N_THREADS];

    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([]() {
            cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
                store(IP, PORT,
                            cirrus::struct_serializer_simple<SIZE>,
                            cirrus::struct_deserializer_simple<SIZE>);

            struct cirrus::Dummy<SIZE> d(42);
            // warm up
            for (uint64_t i = 0; i < 100; ++i) {
                store.put(i, d);
            }

            // barrier
            count++;
            while (count != N_THREADS) {}

            cirrus::TimerFunction tf;
            for (uint64_t i = 0; i < N_MSG; ++i) {
                store.put(i % 100, d);
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
