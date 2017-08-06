#include <stdlib.h>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <cctype>
#include <chrono>
#include <thread>
#include <memory>
#include <random>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "client/TCPClient.h"

static const uint64_t MILLION = 1000000;
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";


/**
  * This benchmark tests the bandwidth of Cirrus when multiple operations
  * are outstanding. It tests how well Cirrus can take advantage of a full
  * pipeline of requests to keep the network hardware busy
  * @param outstanding How many outstanding requests are in flight at any time
  * @param write_size Size of each individual write
  */
template<int SIZE>
void test_outstanding_requests(int outstanding_target) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);
    
    // we have space for outstanding_target futures
    bool send[outstanding_target] = {0};
    typename cirrus::ObjectStore<cirrus::Dummy<SIZE>>::ObjectStorePutFuture futures[outstanding_target];
    struct cirrus::Dummy<SIZE> d(42);

    uint64_t count_completed = 0;
    cirrus::TimerFunction timer("", false);
    for (uint64_t loop = 0; 1; ++loop) {
        // an entry in the futures array is either:
        // 1. initialized by default (begin of execution)
        // 2. completed (try_wait returns true)
        // 3. outstanding (try_wait returns false)
        for (int i = 0; i < outstanding_target; ++i) {
            if (!send[i]) {
                send[i] = true;
                futures[i] = store.put_async(0, d);
            } else if (futures[i].try_wait()) {
                futures[i] = store.put_async(0, d);
                count_completed++;
            }

            if (loop % MILLION == 0) {
                double size_completed_MB = 1.0 * count_completed * SIZE / 1024 / 1024;
                double secs_elapsed = timer.getUsElapsed() / MILLION;
                double bw_MB = 1.0 * size_completed_MB / secs_elapsed;
                std::cout << " Estimated Bandwidth: " << bw_MB
                    << std::endl;
            }
        }
    }
}

auto main() -> int {
    // have 10 requests of 10 bytes outstanding
    test_outstanding_requests<4 * 1024>(30);

    return 0;
}
