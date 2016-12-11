/* Copyright Joao Carreira 2016 */

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

#include "src/object_store/FullBladeObjectStore.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = GB/2;

struct Dummy {
    char data[SIZE];
    int id;
};

void test_sync() {
    sirius::ostore::FullBladeObjectStoreTempl<> store(IP, PORT);

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    try {
        store.put(d.get(), sizeof(Dummy), 1);
    } catch(...) {
        std::cerr << "Error inserting" << std::endl;
    }

    void* d2 = ::operator new(sizeof(Dummy));
    store.get(1, d2);

    // should be 42
    std::cout << "d2.id: " << reinterpret_cast<Dummy*>(d2)->id << std::endl;

    if (reinterpret_cast<Dummy*>(d2)->id != 42) {
        throw std::runtime_error("Wrong value");
    }
}

void test_async() {
    sirius::ostore::FullBladeObjectStoreTempl<> store(IP, PORT);
    
    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    auto future = store.put_async(d.get(), sizeof(Dummy), 1);
    future(false);

    void* d2 = ::operator new(sizeof(Dummy));
    auto future2 = store.get_async(1, d2);

    if (future2 == nullptr)
        throw std::runtime_error("wrong future");

    while (!future2(true)) {
        std::cout << "try wait" << std::endl;
    }
        
    std::cout << "done" << std::endl;

    std::cout << "d2.id: " << reinterpret_cast<Dummy*>(d2)->id << std::endl;
    
    if (reinterpret_cast<Dummy*>(d2)->id != 42) {
        throw std::runtime_error("Wrong value");
    }
}

void test_sync2() {
    sirius::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    try {
        store.put(d.get(), sizeof(Dummy), 1);
    } catch(...) {
        std::cerr << "Error inserting" << std::endl;
    }

    Dummy* d2 = new Dummy;
    store.get(1, d2);

    // should be 42
    std::cout << "d2.id: " << d2->id << std::endl;

    if (d2->id != 42) {
        throw std::runtime_error("Wrong value");
    }
}

auto main() -> int {

    test_sync();
    //test_async();
    // test_sync2();

    return 0;
}

