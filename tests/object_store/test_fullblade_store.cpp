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
#include "common/Serializer.h"
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

/* This function takes an int with value N and makes N copies of it. */
class SerializerVariableSimple : public cirrus::Serializer<int> {
 public:
    uint64_t size(const int& object) const override {
        return object * sizeof(int);
    }

    void serialize(const int& val, void *mem) const override {
        int *int_ptr = reinterpret_cast<int*>(mem);
        for (int i = 0; i < val; i++) {
            *(int_ptr + i) = val;
        }
        return;
    }
};

/* Checks that the N numbers are equal to each other and the number of ints. */
int deserializer_variable_simple(const void* data, unsigned int size) {
    const int *ptr = reinterpret_cast<const int*>(data);
    const int returned_val = *ptr;
    if (returned_val != static_cast<int>(size / sizeof(int))) {
        throw std::runtime_error("Size not equal to the int val");
    }
    for (int i = 0; i < returned_val; i++) {
        if (*(ptr + i) != static_cast<int>(size / sizeof(int))) {
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
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
                      PORT,
                      client.get(),
                      serializer,
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
    auto serializer =
        cirrus::c_int_array_serializer_simple<std::shared_ptr<int>>(4);
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
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
                      PORT,
                      client.get(),
                      serializer,
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
        cirrus::TimerFunction tf;
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
    cirrus::serializer_simple<int> serializer;

    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer,
            cirrus::deserializer_simple<int, sizeof(int)>);

    for (int oid = 0; oid <  10; oid++) {
        store.put(oid, oid);
    }

    // TODO(Tyler): If a test before this one calls put on oid 10, this will
    // not fail.

    // Should fail
    store.get(10);
}

/**
 * Tests a simple asynchronous put and get.
 */
void test_async() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<int> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer,
            cirrus::deserializer_simple<int, sizeof(int)>);
    auto returned_future = store.put_async(0, 42);
    if (!returned_future.get()) {
        throw std::runtime_error("Error occured during async put.");
    }

    auto get_future = store.get_async(0);
    int result = get_future.get();
    if (result != 42) {
        std::cout << "Expected 42 but got: " << result << std::endl;
        throw std::runtime_error("Wrong value returned.");
    }
}

/**
 * Tests multiple concurrent puts and gets. It first puts N items, then ensures
 * all N were successful. It then gets N items, and ensures each value matches.
 * @param N the number of puts and gets to perform.
 */
void test_async_N(int N) {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<int> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer,
            cirrus::deserializer_simple<int, sizeof(int)>);
    std::vector<cirrus::ObjectStore<int>::ObjectStorePutFuture> put_futures;
    std::vector<cirrus::ObjectStore<int>::ObjectStoreGetFuture> get_futures;

    int i;
    for (i = 0; i < N; i++) {
        put_futures.push_back(store.put_async(i, i));
    }
    // Check the success of each put operation
    for (i = 0; i < N; i++) {
        if (!put_futures[i].get()) {
            throw std::runtime_error("Error during an async put.");
        }
    }

    for (i = 0; i < N; i++) {
        get_futures.push_back(store.get_async(i));
    }
    // check the value of each get
    for (i = 0; i < N; i++) {
        int val = get_futures[i].get();
        if (val != i) {
            std::cout << "Expected " << i << " but got " << val << std::endl;
            throw std::runtime_error("Wrong value returned in test_async_N");
        }
    }
}

/**
 * This test tests that get_bulk() and put_bulk() return proper values.
 */
void test_bulk() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<int> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer,
            cirrus::deserializer_simple<int, sizeof(int)>);
    std::vector<int> values(10);
    for (int i = 0; i < 10; i++) {
        values[i] = i;
    }

    store.put_bulk(0, 9, values.data());

    std::vector<int> ret_values(10);
    store.get_bulk(0, 9, ret_values.data());
    for (int i = 0; i < 10; i++) {
        if (ret_values[i] != values[i]) {
            std::cout << "Expected " << i << " but got " << ret_values[i]
                << std::endl;
            throw std::runtime_error("Wrong value returned");
        }
    }
}

/**
 * Tests that remove_bulk correctly removes items from the store.
 */
void test_remove_bulk() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<int> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer,
            cirrus::deserializer_simple<int, sizeof(int)>);
     for (int i = 0; i < 10; i++) {
        store.put(i, i);
     }

    store.remove_bulk(0, 9);
    // Attempt to get all items in the removed range, should fail
    for (int i = 0; i < 10; i++) {
        try {
            store.get(i);
            std::cout << "Exception not thrown after attempting to access item "
                "that should have been removed." << std::endl;
            throw std::runtime_error("No exception when getting removed id.");
        } catch (const cirrus::NoSuchIDException& e) {
        }
    }
}

/**
 * This test tests the remove method. It ensures that you cannot "get"
 * an item if it has been removed from the store.
 */
void test_remove() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<int> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer,
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
    SerializerVariableSimple serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer,
            deserializer_variable_simple);

    for (int i = 1; i < 4; i++) {
        store.put(i, i);
    }
    for (int i = 1; i < 4; i++) {
        store.get(i);
    }
}

/**
 * This test ensures that error messages that would normally be generated
 * during a get are still received during a get bulk.
 */
void test_bulk_nonexistent() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);

    cirrus::serializer_simple<int> serializer;

    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer,
            cirrus::deserializer_simple<int, sizeof(int)>);
    store.put(1, 1);
    std::vector<int> ret_values(10);
    store.get_bulk(1492, 1501, ret_values.data());
}

/**
  * This test ensures that it is possible to share one client between two
  * different stores with different types.
  */
void test_shared_client() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<int> serializer_int;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            serializer_int,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer_dummy;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store2(IP,
            PORT,
            client.get(),
            serializer_dummy,
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
    return;
}

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    IP = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "Starting test." << std::endl;
    std::cout << "Starting synchronous tests." << std::endl;
    test_sync(10);
    std::cout << "Starting test sync no args." << std::endl;
    test_sync();
    test_bulk();

    try {
        test_bulk_nonexistent();
        std::cout << "Exception not thrown when get bulk"
                     " called on nonexistent ID." << std::endl;
        return -1;
    } catch (const cirrus::NoSuchIDException& e) {
    }

    std::cout << "Starting asynchronous tests." << std::endl;
    test_async();
    std::cout << "Test async n." << std::endl;
    test_async_N(10);

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
    std::cout << "test remove bulk" << std::endl;
    test_remove_bulk();
    std::cout << "test shared client" << std::endl;
    test_shared_client();
    std::cout << "test variable sizes" << std::endl;
    test_variable_sizes();
    std::cout << "Test Successful." << std::endl;

    return 0;
}
