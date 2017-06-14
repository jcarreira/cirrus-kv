#include <stdlib.h>
#include <string>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/object_store/object_store_internal.h"
#include "src/common/Exception.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1024*1024;  // One MB
static const uint64_t MILLION = 1000000;


/** This test aims to ensure that when the remote server no longer has room to fulfill
  * all allocations it notifies the client, which will then throw an error message.
  * This test assumes that the server does not have enough room to store one million
  * objects of one MB each (1 TB). This test also ensures that the allocation error does
  * not crash the server as in order for notification of failure to reach the client,
  * the server must have been running to send the message.
  */
void test_exhaustion() {
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT,
                      cirrus::struct_serializer_simple<SIZE>,
                      cirrus::struct_deserializer_simple<SIZE>);
    struct cirrus::Dummy<SIZE> d(42);

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
    } catch (const cirrus::ServerMemoryErrorException & e) {
        return 0;
    }
    /* Exception should be thrown above and caught */
    return -1;
}
