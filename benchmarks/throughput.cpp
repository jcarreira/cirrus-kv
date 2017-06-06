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
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.84";
static const uint64_t numRuns = 2000;
static const uint64_t MILLION = 1000000;

struct Dummy_128 {
    char data[128];
    int id;
};

struct Dummy_4K {
    char data[4096];
    int id;
};

struct Dummy_50 {
    char data[50000];
    int id;
};

struct Dummy_1M {
    char data[1048576];
    int id;
};

struct Dummy_10M {
    char data[10 * 1048576];
    int id;
};


void test_sync_128() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy_128> store(IP, PORT);

    std::unique_ptr<Dummy_128> d = std::make_unique<Dummy_128>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy_128));

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * numRuns; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i % 1000, &mem);

        if (i % (10 * numRuns) == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("throughput_128.log");
    outfile << "throughput 128 test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(Dummy_128)) / (end * 1.0 / MILLION)  << std::endl;

    outfile.close();
}

void test_sync_4K() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy_4K> store(IP, PORT);

    std::unique_ptr<Dummy_4K> d = std::make_unique<Dummy_4K>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy_4K), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy_4K));

    cirrus::Stats stats;
    stats.reserve(MILLION);

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * numRuns; ++i) {
        store.put(d.get(), sizeof(Dummy_4K), i % 1000, &mem);

        if (i % 15000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("throughput_4k.log");
    outfile << "throughput 4k test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(Dummy_4K)) / (end * 1.0 / MILLION)  << std::endl;

    outfile.close();
}

void test_sync_50() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy_50> store(IP, PORT);

    std::unique_ptr<Dummy_50> d = std::make_unique<Dummy_50>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy_50), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy_50));

    cirrus::Stats stats;
    stats.reserve(MILLION);

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * numRuns; ++i) {
        store.put(d.get(), sizeof(Dummy_50), i % 1000, &mem);

        if (i % 15000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("throughput_50K.log");
    outfile << "throughput 50 test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(Dummy_50)) / (end * 1.0 / MILLION)  << std::endl;

    outfile.close();
}

void test_sync_1M() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy_1M> store(IP, PORT);

    std::unique_ptr<Dummy_1M> d = std::make_unique<Dummy_1M>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy_1M), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy_1M));

    cirrus::Stats stats;
    stats.reserve(MILLION);

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * numRuns; ++i) {
        store.put(d.get(), sizeof(Dummy_1M), i % 1000, &mem);

        if (i % 15000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("throughput_1M.log");
    outfile << "throughput 1M test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(Dummy_1M)) / (end * 1.0 / MILLION)  << std::endl;

    outfile.close();
}

void test_sync_10M() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy_10M> store(IP, PORT);

    std::unique_ptr<Dummy_10M> d = std::make_unique<Dummy_10M>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy_10M), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy_10M));

    cirrus::Stats stats;
    stats.reserve(MILLION);

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * numRuns; ++i) {
        store.put(d.get(), sizeof(Dummy_10M), i % 1000, &mem);

        if (i % 15000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("throughput_10M.log");
    outfile << "throughput 10M test" << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(Dummy_10M)) / (end * 1.0 / MILLION)  << std::endl;

    outfile.close();
}

auto main() -> int {
    // test burst of sync writes
    test_sync_128();
    test_sync_4K();
    test_sync_50();
    test_sync_1M();
    test_sync_10M();

    return 0;
}
