#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
#include "client/BladeClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
bool use_rdma_client;

/**
  * Test simple synchronous put and get to/from the object store.
  * Uses simpler objects than test_fullblade_store.
  */
void test_store_simple() {
    std::unique_ptr<cirrus::BladeClient> client = cirrus::getClient(
        use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<int> store(IP, PORT,
                        client.get(),
                        cirrus::serializer_simple<int>,
                        cirrus::deserializer_simple<int, sizeof(int)>);
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

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::parse_mode(argc, argv);
    std::cout << "Test starting" << std::endl;
    test_store_simple();
    std::cout << "Test successful" << std::endl;
    return 0;
}
