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
static const uint64_t GB = (1024*1024*1024);
static const uint64_t MB = (1024*1024);
static const int N_THREADS = 2;
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint32_t SIZE = 1024;

cirrus::TCPClient client;
cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP, PORT,
                    &client,
                    cirrus::struct_serializer_simple<SIZE>,
                    cirrus::struct_deserializer_simple<SIZE>);

/**
  * Tests that behavior is as expected when multiple threads make get and put
  * requests to the remote store. These clients all use the same instance of
  * the store to connect. Currently not working.
  */
void test_mt() {
    cirrus::TimerFunction tf("connect time", true);

    std::thread* threads[N_THREADS];

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    for (int i = 0; i < N_THREADS; ++i) {
        threads[i] = new std::thread([dis, gen]() {
            for (int i = 0; i < 100; ++i) {
                int rnd = std::rand();
                struct cirrus::Dummy<SIZE> d(rnd);

                store.put(1, d);
                cirrus::Dummy<SIZE> d2 = store.get(1);

                if (d2.id != rnd)
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
