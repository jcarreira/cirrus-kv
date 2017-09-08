#include <stdlib.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <cctype>
#include <thread>
#include <memory>
#include <random>
#include <atomic>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "common/Synchronization.h"
#include "client/BladeClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
static const uint64_t MB = (1024*1024);
static const int N_THREADS = 20;
const char PORT[] = "12345";
const char *IP;
static const uint32_t SIZE = 1024;
bool use_rdma_client;

std::atomic<std::uint64_t> total_puts = {0};

/**
 * Checks that the system works properly when multiple clients get and put.
 * These clients each use their own local store and client instances.
 */
void test_multiple_clients() {
    std::cout << "Multiple clients test" << std::endl;

    cirrus::TimerFunction tf("connect time", true);

    std::thread* threads[N_THREADS];

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);
    int start = 0;
    int stop = 10;
    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([dis, gen, start, stop]() {
            std::unique_ptr<cirrus::BladeClient> client =
                cirrus::test_internal::GetClient(
                    use_rdma_client);
            cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
            cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
                store(IP, PORT, client.get(),
                      serializer,
                      cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                          sizeof(cirrus::Dummy<SIZE>)>);
            for (int i = start; i < stop; i++) {
                int rnd = std::rand();
                struct cirrus::Dummy<SIZE> d(rnd);
                store.put(i, d);

                struct cirrus::Dummy<SIZE> d2 = store.get(i);

                if (d2.id != rnd) {
                    std::cout << "Expected " << rnd << " but got " << d2.id
                        << std::endl;
                    throw std::runtime_error("Wrong value returned.");
                }
                total_puts++;
            }
        });
        start += 10;
        stop += 10;
    }

    for (int i = 0; i < N_THREADS; ++i)
        threads[i]->join();

    std::cout << "Test successful" << std::endl;
}

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    IP = cirrus::test_internal::ParseIP(argc, argv);
    test_multiple_clients();

    return 0;
}
