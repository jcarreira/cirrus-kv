/* Copyright Joao Carreira 2016 */

#include <unistd.h>
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
#include <random>
#include <memory>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/iterator/iterator.h"
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";


// #define CHECK_RESULTS

/**
  * Test simple synchronous put and get to/from the object store
  */
void test_simple() {
    cirrus::ostore::FullBladeObjectStoreTempl<> store(IP, PORT);

    for (int i = 0; i < 10; i++) {
      try {
          store.put(&i, sizeof(int), i);
      } catch(...) {
          std::cerr << "Error inserting" << std::endl;
      }
    }

    for (int i = 0; i < 10; i++) {
      int val;
      try {
          if (store.get(i, &val)) {
            if (val != i) {
              throw std::runtime_error("Wrong value");
            }
          } else {
            throw std::runtime_error("Error getting");
          }

      } catch(...) {
          std::cerr << "Error getting" << std::endl;
      }
    }
}

void test_iterator() {
    cirrus::ostore::FullBladeObjectStoreTempl<> store(IP, PORT);

    // Put items in the store
    for (int i = 0; i < 10; i++) {
      try {
          store.put(&i, sizeof(int), i);
      } catch(...) {
          std::cerr << "Error inserting" << std::endl;
      }
    }

    // Use iterator to retrieve
    //inclusive values
    cirrus::StoreIterator iter(0, 9, 0, &store);
    int j = 0;

    for (auto it = iter.begin(); it != iter.end(); it++) {
      int val = *it;
      if (val != j) {
	printf("received %d but expected %d\n", val, j);
        throw std::runtime_error("Wrong value");
      }
      j++;
    }
}

void test_other_format() {
    cirrus::ostore::FullBladeObjectStoreTempl<> store(IP, PORT);

    // Put items in the store
    for (int i = 0; i < 10; i++) {
      try {
          store.put(&i, sizeof(int), i);
      } catch(...) {
          std::cerr << "Error inserting" << std::endl;
      }
    }

    // Use iterator to retrieve
    //inclusive values
    cirrus::StoreIterator iter(0, 9, 0, &store);
    int j = 0;

    for (const auto & it: iter) {
      int val = it;
      if (val != j) {
	printf("received %d but expected %d\n", val, j);
        throw std::runtime_error("Wrong value");
      }
      j++;
    }
}
auto main() -> int {
    test_simple();
    printf("first test complete");
    test_iterator();
    test_other_format();
    return 0;
}
