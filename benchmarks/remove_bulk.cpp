#include <unistd.h>
#include <stdlib.h>
#include <cstdint>
#include <iostream>
#include <cctype>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT

static const uint64_t KB = 1024;
static const uint64_t MB = 1024 * KB;
const char PORT[] = "12345";
const char IP[] = "10.10.49.84";


/**
  * This benchmark is used to determine the latency of
  * removing a set of objects
  */
template <uint64_t SIZE>
void remove_bulk_benchmark(uint64_t num_objs) {
    cirrus::TCPClient client;
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            serializer,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Inserting " << num_objs << " objects" << std::endl;
    for (uint64_t i = 0; i < num_objs; i++) {
        store.put(i, d);
    }

    cirrus::TimerFunction tf;
    store.removeBulk(0, num_objs - 1);
    uint64_t elapsed_us = tf.getUsElapsed();

    std::cout << "Remove bulk took: " << elapsed_us << "us" << std::endl;
}


auto main() -> int {
    remove_bulk_benchmark<100 * KB>(5000);

    return 0;
}
