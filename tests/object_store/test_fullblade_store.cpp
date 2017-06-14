/* Copyright Joao Carreira 2016 */

#include <unistd.h>
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
#include <random>
#include <memory>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/object_store/object_store_internal.h"
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1;

// #define CHECK_RESULTS

/**
  * Tests that simple synchronous put and get to/from the object store
  * works properly.
  */
void test_sync() {
  cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP, PORT,
                      cirrus::struct_serializer_simple<SIZE>,
                      cirrus::struct_deserializer_simple<SIZE>);

  struct cirrus::Dummy<SIZE> d(42);

    try {
        store.put(1, d);
    } catch(...) {
        std::cerr << "Error inserting" << std::endl;
    }

    struct cirrus::Dummy<SIZE> d2 = store.get(1);

    // should be 42
    std::cout << "d2.id: " << d2.id << std::endl;

    if (d2.id != 42) {
        throw std::runtime_error("Wrong value");
    }
}

/**
  * Test a batch of synchronous put and get operations
  * Also record the latencies distributions
  */
void test_sync(int N) {
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT,
                      cirrus::struct_serializer_simple<SIZE>,
                      cirrus::struct_deserializer_simple<SIZE>);
    cirrus::Stats stats;

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    for (int i = 0; i < 100; ++i) {
        store.put(1, d);
    }

    // real benchmark
    for (int i = 0; i < N; ++i) {
        cirrus::TimerFunction tf("", false);
        store.put(1, d);
#ifdef CHECK_RESULTS
        struct cirrus::Dummy<SIZE> d2 = store.get(1);
        if (d2.id != 42) {
            throw std::runtime_error("Wrong value");
        }
#endif

        uint64_t elapsed = tf.getUsElapsed();
        stats.add(elapsed);
    }

    std::cout << "avg: " << stats.avg() << std::endl;
    std::cout << "sd: " << stats.sd() << std::endl;
    std::cout << "99%: " << stats.getPercentile(0.99) << std::endl;
}

auto main() -> int {
    test_sync(1000);
    test_sync();

    return 0;
}
