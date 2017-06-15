#include <unistd.h>
#include <stdlib.h>
#include <iterator>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <cctype>
#include <memory>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/object_store/object_store_internal.h"
#include "src/cache_manager/CacheManager.h"
#include "src/iterator/CirrusIterable.h"


static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const unsigned int SIZE = 1;
const char IP[] = "10.10.49.83";


// #define CHECK_RESULTS
void test_iterator() {
  cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP, PORT,
                      cirrus::struct_serializer_simple<SIZE>,
                      cirrus::struct_deserializer_simple<SIZE>);

  cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, 10);


    // Put items in the store
    for (int i = 0; i < 10; i++) {
      try {
          cirrus::Dummy<SIZE> d(i);
          cm.put(i, d);
      } catch(...) {
          std::cerr << "Error inserting" << std::endl;
      }
    }

    // Use iterator to retrieve
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, 1, 0, 9);

    int j = 0;
    for (auto it = iter.begin(); it != iter.end(); it++) {
      cirrus::Dummy<SIZE> val = *it;
      if (val.id != j) {
        printf("received %d but expected %d\n", val.id, j);
        throw std::runtime_error("Wrong value");
      }
      j++;
    }
}

void test_iterator_alt() {
  cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> store(IP, PORT,
                      cirrus::struct_serializer_simple<SIZE>,
                      cirrus::struct_deserializer_simple<SIZE>);

  cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, 10);


    // Put items in the store
    for (int i = 0; i < 10; i++) {
      try {
          cirrus::Dummy<SIZE> d(i);
          cm.put(i, d);
      } catch(...) {
          std::cerr << "Error inserting" << std::endl;
      }
    }

    // Use iterator to retrieve
    cirrus::CirrusIterable<cirrus::Dummy<SIZE>> iter(&cm, 1, 0, 9);

    int j = 0;
    for (const auto& data : iter) {
      if (data.id != j) {
        printf("received %d but expected %d\n", data.id, j);
        throw std::runtime_error("Wrong value");
      }
      j++;
    }
}

auto main() -> int {
    test_iterator();
    test_iterator_alt();
    return 0;
}
