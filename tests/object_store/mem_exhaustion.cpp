/* Copyright Joao Carreira 2016 */

#include <stdlib.h>
#include <string>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/common/Exception.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1024*1024;  // One MB
static const uint64_t MILLION = 1000000;

struct Dummy {
    char data[SIZE];
    int id;
    Dummy(int id) : id(id) {}
};

/* This function simply copies a struct Dummy into a new portion of memory. */
std::pair<std::unique_ptr<char[]>, unsigned int>
                              struct_serializer_simple(const struct Dummy& v) {
    std::unique_ptr<char[]> ptr(new char[sizeof(struct Dummy)]);
    std::memcpy(ptr.get(), &v, sizeof(struct Dummy));
    return std::make_pair(std::move(ptr), sizeof(struct Dummy));
}

/* Takes a pointer to struct Dummy passed in and returns as object. */
struct Dummy struct_deserializer_simple(void* data, unsigned int /* size */) {
    struct Dummy *ptr = static_cast<struct Dummy*>(data);
    struct Dummy retDummy(ptr->id);
    std::memcpy(&retDummy.data, &(ptr->data), SIZE);
    return retDummy;
}

/** This test aims to ensure that when the remote server no longer has room to fulfill
  * all allocations it notifies the client, which will then throw an error message.
  * This test assumes that the server does not have enough room to store one million
  * objects of one MB each (1 TB). This test also ensures that the allocation error does
  * not crash the server as in order for notification of failure to reach the client,
  * the server must have been running to send the message.
  */
void test_exhaustion() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT,
                      struct_serializer_simple, struct_deserializer_simple);
    struct Dummy d(42);

    // warm up
    std::cout << "Putting 1000" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(i, d);
    }

    std::cout << "Done putting 1000" << std::endl;

    std::cout << "Putting one million objects" << std::endl;
    for (uint64_t i = 0; i < MILLION; ++i) {
        store.put(i, d);
    }
}

auto main() -> int {
    try {
        test_exhaustion();
    } catch (const cirrus::Exception & e) {
        return 0;
    }
    /* Exception should be thrown above and caught */
    return -1;
}
