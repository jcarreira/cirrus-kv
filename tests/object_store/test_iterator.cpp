#include <unistd.h>
#include <stdlib.h>
#include <cstdint>
#include <iostream>
#include <cctype>
#include <memory>

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
      if (data.id != j) {
        std::cout << "received " << data.id << " but expected " << j
                  << std::endl;
        throw std::runtime_error("Wrong value in alternate");
      }
      j++;
    }
}


/**
  * Tests that the cache manager can handle c style arrays.
  */
void test_array() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<int>> store(IP,
                      PORT,
                      client.get(),
                      cirrus::c_array_serializer_simple<int, 4>,
                      cirrus::c_array_deserializer_simple<int, 4>);

    cirrus::LRAddedEvictionPolicy policy(10);
    cirrus::CacheManager<std::shared_ptr<int>> cm(&store, &policy, 10);

    auto int_array = std::shared_ptr<int>(new int[4],
        std::default_delete<int[]>());

    // Put items in the store
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < j; j++) {
            (int_array.get())[j] = (i * 4) + j;
        }
        cm.put(i, int_array);
    }

    // Use iterator to retrieve
    cirrus::CirrusIterable<std::shared_ptr<int>> iter(&cm, 1, 0, 9);

    int i = 0;
    for (const auto& ret_array : iter) {
        for (int j = 0; j < 4; j++) {
            if ((ret_array.get())[j] != (i * 4) + j) {
                std::cout << "Expected: " << (i * 4) + j << " but got "
                    << (ret_array.get())[j]
                    << std::endl;
                throw std::runtime_error("Incorrect value returned");
            }
        }
        i++;
    }
}

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    IP = cirrus::test_internal::ParseIP(argc, argv);
    test_iterator();
    test_iterator_alt();
    test_array();
    std::cout << "Test success" << std::endl;
    return 0;
}
