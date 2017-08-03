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

/* This function takes an int and makes int number copies of it. */
std::pair<std::unique_ptr<char[]>, unsigned int>
                         serializer_variable_simple(const int& v) {
    std::unique_ptr<char[]> ptr(new char[sizeof(int) * v]);
    std::memcpy(ptr.get(), &v, sizeof(int) * v);
    int *int_ptr = reinterpret_cast<int*>(ptr.get());
    for (int i = 0; i < v; i++) {
        *(int_ptr + i) = v;
    }
    return std::make_pair(std::move(ptr), sizeof(int) * v);
}

/* Checks that the int numbers are equal to each other and the size. */
int deserializer_variable_simple(void* data, unsigned int size) {
    int *ptr = reinterpret_cast<int*>(data);
    int returned_val = *ptr;
    if (returned_val != size / sizeof(int)) {
        throw std::runtime_error("Size not equal to the int val");
    }
    for (int i = 0; i < returned_val; i++) {
        if (*(ptr + i) != size / sizeof(int)) {
            throw std::runtime_error("Incorrect value returned");
        }
    }
    int ret;
    std::memcpy(&ret, ptr, sizeof(int));
    return ret;
}




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
  * This test tests the ability to store and retrieve variable sized items.
  */
void test_variable_sizes() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer_variable_simple,
            deserializer_variable_simple);

    for (int i = 1; i < 4; i++) {
        store.put(i, i);
    }
    for (int i = 1; i < 4; i++) {
        store.get(i);
    }
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
    test_variable_sizes();
    std::cout << "Test Successful." << std::endl;
    return 0;
}
