/* Copyright Joao Carreira 2016 */

#include <unistd.h>
#include <stdlib.h>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";


/* This function simply copies an int into a new portion of memory. */
std::pair<std::unique_ptr<void, decltype(&::operator delete)>, unsigned int>
    serializer_simple(const int& v) {
      
    std::unique_ptr<void, decltype(&::operator delete)>
                                   ptr(::operator new (sizeof(int)),
                                   &::operator delete)
    std::memcpy(ptr, &v, sizeof(int));
    return std::make_pair(ptr, sizeof(int));
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
void test_store_simple() {
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT,
                        serializer_simple, deserializer_simple);
    for (int oid = 0; oid <  10; oid++) {
      store.put(oid, oid);
    }

    for (int oid = 0; oid <  10; oid++) {
      int retval = store.get(oid);
      if (retval != oid) {
        throw std::runtime_error("Wrong value returned.");
      }
    }
}

auto main() -> int {
    printf("test starting\n");
    test_store_simple();
    return 0;
}
