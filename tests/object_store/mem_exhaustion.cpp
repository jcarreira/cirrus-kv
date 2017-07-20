#include <stdlib.h>
#include <string>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "common/Exception.h"
#include "client/BladeClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char *IP;
static const uint32_t SIZE = 1024*1024;  // One MB
static const uint64_t MILLION = 1000000;
bool use_rdma_client;

/**
 * This test aims to ensure that when the remote server no longer has room to fulfill
 * all allocations it notifies the client, which will then throw an error message.
 * This test assumes that the server does not have enough room to store one million
 * objects of one MB each (1 TB). This test also ensures that the allocation error does
 * not crash the server as in order for notification of failure to reach the client,
 * the server must have been running to send the message.
 */
void test_exhaustion() {
    std::unique_ptr<cirrus::BladeClient> client = cirrus::getClient(
        use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, client.get(),
                cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
                cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                    sizeof(cirrus::Dummy<SIZE>)>);
    struct cirrus::Dummy<SIZE> d(42);

    std::cout << "Putting one million objects" << std::endl;
    for (uint64_t i = 0; i < MILLION; ++i) {
        store.put(i, d);
    }
}

/**
 * Test to ensure that the remove method works as expected. Ensures that when
 * the server is at capacity, an item can be removed to make more room.
 */
void test_exhaustion_remove() {
    std::unique_ptr<cirrus::BladeClient> client = cirrus::getClient(
        use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, client.get(),
                cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
                cirrus::deserializer_simple<cirrus::Dummy<SIZE>, SIZE>);
    struct cirrus::Dummy<SIZE> d(42);

    std::cout << "Putting one million objects" << std::endl;
    uint64_t i;
    for (i = 0; i < MILLION; ++i) {
        try {
            store.put(i, d);
        } catch (const cirrus::ServerMemoryErrorException& e) {
            break;
        }
    }
    std::cout << "Removing an object to make room." << std::endl;
    // Remove an object to make room
    store.remove(0);
    std::cout << "Attempting to insert an object in the newly freed space."
        << std::endl;
    // Attempt to use the newly freed space
    store.put(i, d);
}

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::parse_mode(argc, argv);
    IP = cirrus::parse_ip(argc, argv);
    test_exhaustion_remove();
    try {
        test_exhaustion();
    } catch (const cirrus::ServerMemoryErrorException& e) {
        std::cout << "Test successful" << std::endl;
        return 0;
    }
    /* Exception should be thrown above and caught */
    return -1;
}
