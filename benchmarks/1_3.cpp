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

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "client/TCPClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t MILLION = 1000000;
static const int N_THREADS = 10;
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint32_t SIZE = 128;
static const uint64_t N_MSG = 100;

uint64_t total_puts = 0;
uint64_t total_time = 0;
    
std::atomic<int> count;

void print_results(std::ostream& out) {
    out << "Total time: " << total_time << std::endl;
    out << "Average (us) per msg: "
        << total_time / N_THREADS / N_MSG << std::endl;
    out << "MSG/s: "
        << N_MSG * N_THREADS / (total_time * 1.0 / MILLION) << std::endl;
}

/**
  * Test the performance of the system when multiple clients
  * are connected to the same store.
  * Create ten new threads,
  * each of which connects to the store and sends N_MSG puts spread across
  * 100 different object ids
  */
void test_multiple_clients() {
    std::thread* threads[N_THREADS];

    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([]() {
            cirrus::TCPClient client;
            cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
                store(IP, PORT, &client,
                    cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
                    cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                        sizeof(cirrus::Dummy<SIZE>)>);
            std::cout << "New thread connected." << std::endl;
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

    print_results(std::cout);

    std::ofstream outfile;
    outfile.open("1_3.log");
    print_results(outfile);
}

auto main() -> int {
    test_multiple_clients();

    return 0;
}
