#include <stdlib.h>
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <thread>
#include <memory>
#include <random>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "client/BladeClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
static const uint64_t MB = (1024*1024);
static const int N_THREADS = 2;
const char PORT[] = "12345";
const char *IP;
static const uint32_t SIZE = 1024;
bool use_rdma_client;

/**
  * Tests that behavior is as expected when multiple threads make get and put
  * requests to the remote store. These threads all use the same instance of
  * the store to connect.
  */
void test_mt() {
    cirrus::TimerFunction tf("connect time", true);

    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
                        PORT,
                        client.get(),
                        cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
                        cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                            sizeof(cirrus::Dummy<SIZE>)>);

    std::thread* threads[N_THREADS];

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);
    int start = 0;
    int stop = 10;
    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([dis, gen, start, stop, & store]() {
            for (int i = start; i < stop; i++) {
                int rnd = std::rand();
                struct cirrus::Dummy<SIZE> d(rnd);

                store.put(i, d);
                cirrus::Dummy<SIZE> d2 = store.get(i);

                if (d2.id != rnd)
                    throw std::runtime_error("Incorrect value returned.");
            }
        });
        start += 10;
        stop += 10;
    }

    for (int i = 0; i < N_THREADS; ++i)
        threads[i]->join();
}

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    IP = cirrus::test_internal::ParseIP(argc, argv);
    test_mt();
    std::cout << "Test success" << std::endl;
    return 0;
}
