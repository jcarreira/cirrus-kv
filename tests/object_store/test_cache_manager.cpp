#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "cache_manager/CacheManager.h"
#include "utils/Time.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

// TODO: Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";

/**
  * Test simple synchronous put and get to/from the object store.
  * Uses simpler objects than test_fullblade_store.
  */
void test_cache_manager_simple() {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client,
                        cirrus::serializer_simple,
                        cirrus::deserializer_simple);

    cirrus::CacheManager<int> cm(&store, 10);
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
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client,
                        cirrus::serializer_simple,
                        cirrus::deserializer_simple);

    cirrus::CacheManager<int> cm(&store, 10);
    for (int oid = 0; oid <  10; oid++) {
        cm.put(oid, oid);
    }

    // Should fail
    cm.get(10);
}

/**
  * This test tests the behavior of the cache manager when the cache is at
  * capacity. At moment, the cache should throw
  * a cirrus::CacheCapacityException.
  */
void test_capacity() {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client,
                        cirrus::serializer_simple,
                        cirrus::deserializer_simple);

    cirrus::CacheManager<int> cm(&store, 10);
    for (int oid = 0; oid <  15; oid++) {
        cm.put(oid, oid);
    }

    for (int oid = 0; oid < 10; oid++) {
        cm.get(oid);
    }

    // Should fail
    cm.get(10);
}

/**
  * This test tests the behavior of the cache manager when instantiated with
  * a maximum capacity of zero. Should throw cirrus::CacheCapacityException.
  */
void test_instantiation() {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT, client,
                        cirrus::serializer_simple,
                        cirrus::deserializer_simple);

    cirrus::CacheManager<int> cm(&store, 0);
}

auto main() -> int {
    std::cout << "test starting" << std::endl;
    test_cache_manager_simple();

    try {
        test_capacity();
        std::cout << "Exception not thrown when cache"
                     " capacity exceeded." << std::endl;
        return -1;
    } catch (const cirrus::CacheCapacityException& e) {
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

    return 0;
}
