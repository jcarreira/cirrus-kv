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
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1;

struct Dummy {
    char data[SIZE];
    int id;
    Dummy(int id) : id(id) {}
};

/* This function simply copies a struct Dummy into a new portion of memory. */
std::pair<std::unique_ptr<char[]>, unsigned int>
                              struct_serializer_simple(const struct Dummy& v) {
    std::unique_ptr<char[]> ptr(new char[sizeof(struct Dummy)]);
    std::memcpy(ptr.get(), &v, sizeof(struct Dummy));
    return std::make_pair(std::move(ptr), sizeof(struct Dummy));
}

/* Takes a pointer to struct Dummy passed in and returns as object. */
struct Dummy struct_deserializer_simple(void* data, unsigned int /* size */) {
    struct Dummy *ptr = static_cast<struct Dummy*>(data);
    struct Dummy retDummy(ptr->id);
    std::memcpy(&retDummy.data, &(ptr->data), SIZE);
    return retDummy;
}

// #define CHECK_RESULTS

/**
  * Tests that simple synchronous put and get to/from the object store
  * works properly.
  */
void test_sync() {
  cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT,
                      struct_serializer_simple, struct_deserializer_simple);

  struct Dummy d(42);

    try {
        store.put(1, d);
    } catch(...) {
        std::cerr << "Error inserting" << std::endl;
    }

    struct Dummy d2 = store.get(1);

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
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT,
                      struct_serializer_simple, struct_deserializer_simple);
    cirrus::Stats stats;

    struct Dummy d;
    d.id = 42;

    // warm up
    for (int i = 0; i < 100; ++i) {
        store.put(1, d);
    }

    // real benchmark
    for (int i = 0; i < N; ++i) {
        cirrus::TimerFunction tf("", false);
        store.put(1, d);
#ifdef CHECK_RESULTS
        struct Dummy d2 = store.get(1);
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
    // TODO: Add back the async tests, visible in git history
    test_sync(1000);
    test_sync();

    return 0;
}
