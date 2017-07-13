#include <unistd.h>
#include <stdlib.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <cctype>
#include <memory>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "common/Exception.h"
#include "utils/Time.h"
#include "utils/Stats.h"
#include "client/RDMAClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
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
    cirrus::RDMAClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
                      PORT,
                      &client,
                      cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
                      cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                        sizeof(cirrus::Dummy<SIZE>)>);

    struct cirrus::Dummy<SIZE> d(42);

    store.put(1, d);

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
    cirrus::RDMAClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
                PORT,
                &client,
                cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
                cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                    sizeof(cirrus::Dummy<SIZE>)>);

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

/**
  * This test tests the behavior of the store when attempting to
  * get an ID that has never been put. Should throw a cirrus::NoSuchIDException.
  */
void test_nonexistent_get() {
    cirrus::RDMAClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, &client,
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    for (int oid = 0; oid <  10; oid++) {
        store.put(oid, oid);
    }

    // Should fail
    store.get(10);
}

/**
  * This test tests the remove method. It ensures that you cannot "get"
  * an item if it has been removed from the store.
  */
void test_remove() {
    cirrus::RDMAClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, &client,
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    store.put(0, 42);

    store.remove(0);

    // Should fail
    int i = store.get(0);
    std::cout << "Received following value incorrectly: " << i << std::endl;
}

auto main() -> int {
    test_sync(10);
    test_sync();

    try {
        test_nonexistent_get();
        std::cout << "Exception not thrown when get"
                     " called on nonexistent ID." << std::endl;
        return -1;
    } catch (const cirrus::NoSuchIDException& e) {
    }
    std::cout << "Test remove starting." << std::endl;
    try {
        test_remove();
        std::cout << "Exception not thrown when get"
                     " called on removed ID." << std::endl;
        return -1;
    } catch (const cirrus::NoSuchIDException& e) {
    }

    std::cout << "Test Successful." << std::endl;
    return 0;
}
