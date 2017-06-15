#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/object_store/object_store_internal.h"
#include "src/cache_manager/CacheManager.h"
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";

/**
  * Test simple synchronous put and get to/from the object store.
  * Uses simpler objects than test_fullblade_store.
  */
void test_cache_manager_simple() {
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT,
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

void test_nonexistent_get() {
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT,
                        cirrus::serializer_simple,
                        cirrus::deserializer_simple);

    cirrus::CacheManager<int> cm(&store, 10);
    for (int oid = 0; oid <  10; oid++) {
        cm.put(oid, oid);
    }

    // Should fail
    cm.get(10);
}

void test_capacity() {
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT,
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

void test_instantiation() {
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT,
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
    } catch (const cirrus::CacheCapacityException & e) {
    }

    try {
        test_instantiation();
        std::cout << "Exception not thrown when cache"
                     " capacity set to zero." << std::endl;
        return -1;
    } catch (const cirrus::CacheCapacityException & e) {
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
