#include <unistd.h>
#include <stdlib.h>
#include <cstdint>
#include <iostream>
#include <cctype>
#include <memory>
#include <chrono>
#include <thread>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "cache_manager/CacheManager.h"
#include "iterator/CirrusIterable.h"
#include "client/BladeClient.h"
#include "cache_manager/LRAddedEvictionPolicy.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const unsigned int SIZE = 1;
const char *IP;
bool use_rdma_client;

/**
  * This test ensures that items can be properly retrieved using
  * the iterator interface.
  */
void test_iterator() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            client.get(),
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, 10);

    // Put items in the store
    for (int i = 0; i < 10; i++) {
        cirrus::Dummy<SIZE> d(i);
        cm.put(i, d);
    }

    // Use iterator to retrieve
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, 1, 0, 9);

    int j = 0;
    for (auto it = iter.begin(); it != iter.end(); it++) {
      cirrus::Dummy<SIZE> val = *it;
      if (val.id != j) {
        std::cout << "received " << val.id << " but expected " << j
                  << std::endl;
        throw std::runtime_error("Wrong value");
      }
      j++;
    }
}


/**
  * This test ensures that items can be properly retrieved using
  * the iterator interface, but using c++ range based for loop.
  */
void test_iterator_alt() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            client.get(),
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, 10);


    // Put items in the store
    for (int i = 0; i < 10; i++) {
        cirrus::Dummy<SIZE> d(i);
        cm.put(i, d);
    }

    // Use iterator to retrieve
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, 1, 0, 9);

    int j = 0;
    for (const auto& data : iter) {
        std::cout << "Retrieved an item" << std::endl;
        if (data.id != j) {
            std::cout << "received " << data.id << " but expected " << j
                  << std::endl;
            throw std::runtime_error("Wrong value in alternate");
        }
        j++;
    }
}

/**
 * This test ensures that random prefetching works as expected.
 */
void test_random_prefetching() {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP,
            PORT,
            &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, 10);


    // Put items in the store
    for (int i = 0; i < 10; i++) {
        cirrus::Dummy<SIZE> d(i);
        cm.put(i, d);
    }

    // Use iterator to retrieve
    // Read 9 ahead so that all objects will be stashed
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, 9, 0, 9);
    iter.setMode(cirrus::CirrusIterable<cirrus::Dummy<SIZE>>::kUnOrdered);
    int j = 0;
    auto start = std::chrono::system_clock::now();
    auto end = std::chrono::system_clock::now();
    auto duration = end - start;

    for (const auto& data : iter) {
        end = std::chrono::system_clock::now();
        if (j != 0) {
            // Time how long this last loop took
            duration = end - start;
            auto duration_micro =
                std::chrono::duration_cast<std::chrono::microseconds>(duration);
            if (duration_micro.count() > 5) {
                std::cout << "Elapsed is: " << duration_micro.count()
                    << std::endl;
                throw std::runtime_error("Get took too long, likely not "
                        "prefetched");
            }
        } else {
            // Sleep a bit to let the prefetches finish
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        start = std::chrono::system_clock::now();
        j++;
    }
    if (j != 10) {
        std::cout << j << " items iterated over instead of ten." << std::endl;
        throw std::runtime_error("Either greater or fewer than ten items were "
                "iterated over, meaning either that some items were skipped "
                "or that some were repeated.");
    }
}

auto main(int argc, char *argv[]) -> int {
    std::cout << "Test starting" << std::endl;
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    IP = cirrus::test_internal::ParseIP(argc, argv);
    test_iterator();
    test_iterator_alt();
    test_random_prefetching();
    std::cout << "Test success" << std::endl;
    return 0;
}
