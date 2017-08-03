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
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
#include "client/BladeClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char *IP;
static const uint32_t SIZE = 1;
bool use_rdma_client;

// #define CHECK_RESULTS

/**
  * Tests that simple synchronous put and get to/from the object store
  * works properly.
  */
void test_sync() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
                      PORT,
                      client.get(),
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
  * Tests that the store can handle c style arrays.
  */
void test_sync_array() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);

    auto deserializer = cirrus::c_array_deserializer_simple<int>(4);
    auto serializer = cirrus::c_array_serializer_simple<int>(4);
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<int>> store(IP,
                      PORT,
                      client.get(),
                      serializer,
                      deserializer);

    auto int_array = std::shared_ptr<int>(new int[4],
        std::default_delete<int[]>());

    for (int i = 0; i < 4; i++) {
        (int_array.get())[i] = i;
    }

    store.put(1, int_array);

    std::shared_ptr<int> ret_array = store.get(1);

    for (int i = 0; i < 4; i++) {
        if ((ret_array.get())[i] != i) {
            std::cout << "Expected: " << i << " but got "
                << (ret_array.get())[i]
                << std::endl;
            throw std::runtime_error("Incorrect value returned");
        }
    }
}


/**
  * Test a batch of synchronous put and get operations
  * Also record the latencies distributions
  */
void test_sync(int N) {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
                PORT,
                client.get(),
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
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
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
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    store.put(0, 42);

    store.remove(0);

    // Should fail
    int i = store.get(0);
    std::cout << "Received following value incorrectly: " << i << std::endl;
}

/**
  * This test ensures that it is possible to share one client between two
  * different stores with different types.
  */
void test_shared_client() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);

    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store2(IP,
            PORT,
            client.get(),
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    for (int oid = 0; oid <  10; oid++) {
        store.put(oid, oid);
    }

    for (int i = 10; i <  20; i++) {
        struct cirrus::Dummy<SIZE> d(i);
        store2.put(i, d);
    }

    for (int i = 0; i < 10; i++) {
        int retval = store.get(i);
        if (retval != i) {
            std::cout << "Expected " << i << " but got " << retval << std::endl;
            throw std::runtime_error("wrong value returned");
        }
        auto retstruct = store2.get(i + 10);
        if (retstruct.id != i + 10) {
            std::cout << "Expected " << i + 10 << " but got " << retstruct.id
                << std::endl;
            throw std::runtime_error("wrong value returned");
        }
    }

    // Should fail
    store.get(10);
}

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    IP = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "Starting test." << std::endl;
    test_sync(10);
    std::cout << "Starting test sync no args." << std::endl;
    test_sync();
    std::cout << "Starting test of c array." << std::endl;
    test_sync_array();
    std::cout << "Testing nonexistent get." << std::endl;
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
    test_shared_client();
    std::cout << "Test Successful." << std::endl;
    return 0;
}
