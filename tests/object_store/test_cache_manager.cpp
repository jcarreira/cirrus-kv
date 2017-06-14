#include <unistd.h>
#include <stdlib.h>

#include "src/object_store/FullBladeObjectStore.h"
// #include "src/object_store/object_store_internal.h"
#include "src/cache_manager/CacheManager.h"
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";

/* This function simply copies an int into a new portion of memory. */
std::pair<std::unique_ptr<char[]>, unsigned int>
    serializer_simple(const int& v) {

    std::unique_ptr<char[]> ptr(new char[sizeof(int)]);
    std::memcpy(ptr.get(), &v, sizeof(int));
    return std::make_pair(std::move(ptr), sizeof(int));
}

/* Takes an int passed in and returns as object. */
int deserializer_simple(void* data, unsigned int /* size */) {
    int *ptr = reinterpret_cast<int *>(data);
    int retval = *ptr;
    return retval;
}

/**
  * Test simple synchronous put and get to/from the object store.
  * Uses simpler objects than test_fullblade_store.
  */
void test_cache_manager_simple() {
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT,
                        serializer_simple,
                        deserializer_simple);

    cirrus::CacheManager<int> cm(&store, 10);
    for (int oid = 0; oid <  10; oid++) {
        cm.put(oid, oid);
    }

    for (int oid = 0; oid <  10; oid++) {
        int retval = cm.get(oid);
        if (retval != oid) {
            printf("expected %d but got %d\n\n\n\n", oid, retval);
            throw std::runtime_error("Wrong value returned.");
        }
    }
}

auto main() -> int {
    printf("test starting\n");
    test_cache_manager_simple();
    return 0;
}
