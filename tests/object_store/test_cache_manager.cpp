#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <thread>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "cache_manager/CacheManager.h"
#include "utils/CirrusTime.h"
#include "cache_manager/LRAddedEvictionPolicy.h"
#include "utils/Stats.h"
#include "cache_manager/PrefetchPolicy.h"
using ObjectID = uint64_t;
#include "client/BladeClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char *IP;
bool use_rdma_client;


/**
 * A simple custom prefetching policy that prefetches in a downwards fashion.
 */
template<class T>
class SimpleCustomPolicy : public cirrus::PrefetchPolicy<T> {
 private:
    ObjectID first;
    ObjectID last;
    uint64_t read_ahead = 1;

 public:
    /**
     * Counterpart to the CacheManager's get method. Returns a list of items
     * to prefetch.
     * @param id the ObjectID being retrieved.
     * @param obj unused
     * @return a vector of ObjectIDs to be prefetched.
     */
    std::vector<ObjectID> get(const ObjectID& id,
        const T& /* obj */) override {
        if (id < first || id > last) {
            throw cirrus::Exception("Attempting to get id outside of "
                    "continuous range present at time of prefetch  mode "
                    "specification.");
        }
        // Math to make sure that prefetching loops back around
        // Formula is:
        // val = ((oid + i) - first) % (last - first + 1)) + first
        std::vector<ObjectID> to_return;
        to_return.reserve(read_ahead);
        for (unsigned int i = 1; i <= read_ahead; i++) {
            ObjectID tentative_fetch = id - i;
            ObjectID shifted = tentative_fetch - first;
            ObjectID modded = shifted % (last - first + 1);
            ObjectID to_prefetch = modded + first;
            // If attempting to prefetch the id that was just retrieved,
            // then a full circle has been completed, so no further
            // prefetching is necessary
            if (to_prefetch == id) {
                break;
            }
            to_return.push_back(to_prefetch);
        }
        return to_return;
    }
    /**
     * Sets the range that this policy will use.
     * @param first_ first objectID in a continuous range
     * @param last_ last objectID that will be used
     */
    void setRange(ObjectID first_, ObjectID last_) {
        first = first_;
        last = last_;
    }
};

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

    auto deserializer = cirrus::c_array_deserializer_simple<int>(4);
    auto serializer = cirrus::c_array_serializer_simple<int>(4);
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<int>> store(IP,
                      PORT,
                      client.get(),
                      serializer,
                      deserializer);

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
  * This test tests the behavior of the cache manager when the cache is in
  * linear prefetching mode.
  */
void test_linear_prefetch() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);

    for (int i = 0; i < 5; i++) {
        cm.put(i, i);
    }

    cm.setMode(cirrus::CacheManager<int>::kOrdered, 0, 4, 5);
    cm.get(0);
    // Sleep for a bit to allow the items to be retrieved
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Ensure that oid 1 was prefetched
    cm.setMode(cirrus::CacheManager<int>::kNone);
    auto start = std::chrono::system_clock::now();
    cm.get(1);
    auto end = std::chrono::system_clock::now();
    auto duration = end - start;
    auto duration_micro =
        std::chrono::duration_cast<std::chrono::microseconds>(duration);
    // Use a threshold of ten because things are slow on the first timed access
    // Typically takes 4-5 µs but sometimes spikes to 15
    // Safe as a store.get takes 200+ µs on RDMA and TCP

    if (duration_micro.count() > 30) {
        std::cout << "Elapsed is: " << duration_micro.count() << std::endl;
        throw std::runtime_error("Get took too long likely not prefetched.");
    }
}

/**
  * This test tests that custom prefetching policies work properly.
  */
void test_custom_prefetch() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);

    for (int i = 0; i < 4; i++) {
        cm.put(i, i);
    }

    SimpleCustomPolicy<int> prefetch_policy;
    prefetch_policy.setRange(0, 3);
    cm.setMode(cirrus::CacheManager<int>::kCustom, &prefetch_policy);
    cm.get(0);
    // Sleep for a bit to allow the items to be retrieved
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    cm.setMode(cirrus::CacheManager<int>::kNone);
    // Ensure that oid 3 was prefetched
    auto start = std::chrono::system_clock::now();
    cm.get(3);
    auto end = std::chrono::system_clock::now();
    auto duration = end - start;
    auto duration_micro =
        std::chrono::duration_cast<std::chrono::microseconds>(duration);
    if (duration_micro.count() > 150) {
        std::cout << "Elapsed is: " << duration_micro.count() << std::endl;
        throw std::runtime_error("Custom get took too long, "
            "likely not prefetched.");
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
 * Tests to ensure that when the cache manager's removeBulk method is called
 * the objects are removed from the store as well.
 */
void test_remove_bulk() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);

    for (int i = 0; i < 10; i++) {
        cm.put(i, i);
    }

    // Remove the items
    cm.removeBulk(0, 9);

    // Attempt to get all items in the removed range, should fail
    for (int i = 0; i < 10; i++) {
        try {
            cm.get(i);
            std::cout << "Exception not thrown after attempting to access item "
                "that should have been removed." << std::endl;
            throw std::runtime_error("No exception when getting removed id.");
        } catch (const cirrus::NoSuchIDException& e) {
        }
    }
}

/**
 * Tests to ensure that when prefetchBulk is called the items are actually
 * prefetched.
 */
void test_prefetch_bulk() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);

    for (int i = 0; i < 10; i++) {
        cm.put(i, i);
    }

    cm.prefetchBulk(0, 9);

    // Sleep for a bit to allow the items to be retrieved
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Fail if any of the gets take more than a few microseconds
    for (int i = 0; i < 10; i++) {
        auto start = std::chrono::system_clock::now();
        cm.get(i);
        auto end = std::chrono::system_clock::now();
        auto duration = end - start;
        auto duration_micro =
            std::chrono::duration_cast<std::chrono::microseconds>(duration);
        // First get is normally slow for some reason
        if (i != 0) {
            if (duration_micro.count() > 5) {
                std::cout << "Elapsed is: "
                    << duration_micro.count() << std::endl;
                throw std::runtime_error("Prefetch bulk get took too long, "
                    "likely not prefetched.");
            }
        }
    }
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
    for (unsigned int i = 0; i < 10; i++) {
        auto ret_vec = policy.get(i);
        if (ret_vec.size() != 0) {
            std::cout << i << "id where error occured" << std::endl;
            throw std::runtime_error("Item evicted when cache not full");
         }
    }
    for (unsigned int i = 10; i < 20; i++) {
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

/**
 * This test tests that get_bulk() and put_bulk() return proper values.
 */
void test_bulk() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);

    std::vector<int> values(10);
    for (int i = 0; i < 10; i++) {
        values[i] = i;
    }

    cm.put_bulk(0, 9, values.data());

    std::vector<int> ret_values(10);
    cm.get_bulk(0, 9, ret_values.data());
    for (int i = 0; i < 10; i++) {
        if (ret_values[i] != values[i]) {
            std::cout << "Expected " << i << " but got " << ret_values[i]
                << std::endl;
            throw std::runtime_error("Wrong value returned");
        }
    }
}

/**
 * This test ensures that error messages that would normally be generated
 * during a get are still received during a get bulk.
 */
void test_bulk_nonexistent() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client.get(),
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<int> cm(&store, &policy, 10);
    cm.put(1, 1);
    std::vector<int> ret_values(10);
    cm.get_bulk(1492, 1501, ret_values.data());
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

    test_remove_bulk();

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

    test_prefetch_bulk();
    test_bulk();

    try {
        test_bulk_nonexistent();
        std::cout << "No exception thrown when get bulk called on nonexistent "
            " ID." << std::endl;
        return -1;
    } catch (const cirrus::NoSuchIDException & e) {
    }

    test_linear_prefetch();
    test_custom_prefetch();
    std::cout << "test successful" << std::endl;
    return 0;
}
