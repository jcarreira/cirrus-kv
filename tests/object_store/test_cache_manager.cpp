#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "cache_manager/CacheManager.h"
#include "utils/CirrusTime.h"
#include "cache_manager/LRAddedEvictionPolicy.h"
#include "utils/Stats.h"
#include "client/BladeClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char *IP;
bool use_rdma_client;

/**
  * Test simple synchronous put and get to/from the object store.
  * Uses simpler objects than test_fullblade_store.
  */
void test_cache_manager_simple() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);
    for (int oid = 0; oid <  10; oid++) {
        cm.put(oid, oid);
    }

    for (int oid = 0; oid <  10; oid++) {
        int retval = cm.get(oid);
        if (retval != oid) {
            throw std::runtime_error("Wrong value returned.");
        }
    }
}

/**
  * This test tests the behavior of the cache manager when attempting to
  * get an ID that has never been put. Should throw a cirrus::NoSuchIDException.
  */
void test_nonexistent_get() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);
    for (int oid = 0; oid <  10; oid++) {
        cm.put(oid, oid);
    }

    // Should fail
    cm.get(1492);
}

/**
  * Tests that the cache manager can handle c style arrays.
  */
void test_array() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<int>> store(IP,
                      PORT,
                      client.get(),
                      cirrus::c_array_serializer_simple<int, 4>,
                      cirrus::c_array_deserializer_simple<int, 4>);

    auto int_array = std::shared_ptr<int>(new int[4],
        std::default_delete<int[]>());

    for (int i = 0; i < 4; i ++) {
        (int_array.get())[i] = i;
    }


    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<std::shared_ptr<int>> cm(&store, &policy, 10);
    cm.put(1, int_array);

    std::shared_ptr<int> ret_array = cm.get(1);

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
  * This test tests the behavior of the cache manager when the cache is at
  * capacity. Should not exceed capacity as the policy should remove items.
  */
void test_capacity() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);
    for (int oid = 0; oid <  15; oid++) {
        cm.put(oid, oid);
    }

    for (int oid = 0; oid < 10; oid++) {
        cm.get(oid);
    }

    // Should not fail
    cm.get(10);
}

/**
 * Tests to ensure that when the cache manager's remove() method is called
 * the given object is removed from the store as well.
 */
void test_remove() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);

    cm.put(0, 0);

    // Remove item
    cm.remove(0);

    // Attempt to get item, this should fail
    cm.get(0);
}
/**
  * This test tests the behavior of the cache manager when instantiated with
  * a maximum capacity of zero. Should throw cirrus::CacheCapacityException.
  */
void test_instantiation() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 0);
}

/**
 * This test checks the behavior of the LRAddedEvictionPolicy by ensuring
 * that it always returns the one oldest item, and only does so when at
 * capacity.
 */
void test_lradded() {
    cirrus::LRAddedEvictionPolicy policy(10);
    int i;
    for (i = 0; i < 10; i++) {
        auto ret_vec = policy.get(i);
        if (ret_vec.size() != 0) {
            std::cout << i << "id where error occured" << std::endl;
            throw std::runtime_error("Item evicted when cache not full");
         }
    }
    for (i = 10; i < 20; i++) {
        auto ret_vec = policy.get(i);
        if (ret_vec.size() != 1) {
            throw std::runtime_error("More or less than one item returned "
                    "when at capacity.");
        } else if (ret_vec.front() != i - 10) {
            throw std::runtime_error("Item returned was not oldest in "
                    "the cache.");
        }
    }
}

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    IP = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "test starting" << std::endl;
    test_cache_manager_simple();
    test_array();

    try {
        test_capacity();
    } catch (const cirrus::CacheCapacityException& e) {
        std::cout << "Cache capacity exceeded when item should have been "
                     " removed by eviction policy." << std::endl;
        return -1;
    }

    try {
        test_remove();
        std::cout << "Exception not thrown after attempting to access item "
            "that should have been removed." << std::endl;
        return -1;
    } catch (const cirrus::NoSuchIDException& e) {
    }

    try {
        test_instantiation();
        std::cout << "Exception not thrown when cache"
                     " capacity set to zero." << std::endl;
        return -1;
    } catch (const cirrus::CacheCapacityException& e) {
    }

    try {
        test_nonexistent_get();
        std::cout << "Exception not thrown when get"
                     " called on nonexistent ID." << std::endl;
        return -1;
    } catch (const cirrus::NoSuchIDException & e) {
    }
    test_lradded();
    std::cout << "test successful" << std::endl;
    return 0;
}
