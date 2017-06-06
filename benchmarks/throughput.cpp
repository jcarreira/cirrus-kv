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
const char IP[] = "10.10.49.83";
static const uint64_t MILLION = 1000000;

struct Dummy_128 {
    char data[128];
    int id;
};

struct Dummy_256 {
    char data[256];
    int id;
};

struct Dummy_512 {
    char data[512];
    int id;
};

struct Dummy_1K {
    char data[1024];
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


void test_sync_128() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy_128));

    cirrus::Stats stats;
    stats.reserve(MILLION);

    std::cout << "Measuring latencies.." << std::endl;
    for (uint64_t i = 0; i < MILLION; ++i) {
        cirrus::TimerFunction tf;
        store.put(d.get(), sizeof(Dummy_128), i % 1000, &mem);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * MILLION; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i % 1000, &mem);

        if (i % 100000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("throughput_128.log");
    outfile << "throughput 128 test" << std::endl;
    outfile << "count: " << stats.getCount() << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(Dummy_128)) / (end * 1.0 / MILLION)  << std::endl;

    outfile.close();
}

void test_sync_128() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy_128));

    cirrus::Stats stats;
    stats.reserve(MILLION);

    std::cout << "Measuring latencies.." << std::endl;
    for (uint64_t i = 0; i < MILLION; ++i) {
        cirrus::TimerFunction tf;
        store.put(d.get(), sizeof(Dummy_128), i % 1000, &mem);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * MILLION; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i % 1000, &mem);

        if (i % 100000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("throughput_128.log");
    outfile << "throughput 128 test" << std::endl;
    outfile << "count: " << stats.getCount() << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(Dummy_128)) / (end * 1.0 / MILLION)  << std::endl;

    outfile.close();
}

void test_sync_128() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy_128));

    cirrus::Stats stats;
    stats.reserve(MILLION);

    std::cout << "Measuring latencies.." << std::endl;
    for (uint64_t i = 0; i < MILLION; ++i) {
        cirrus::TimerFunction tf;
        store.put(d.get(), sizeof(Dummy_128), i % 1000, &mem);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * MILLION; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i % 1000, &mem);

        if (i % 100000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("throughput_128.log");
    outfile << "throughput 128 test" << std::endl;
    outfile << "count: " << stats.getCount() << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(Dummy_128)) / (end * 1.0 / MILLION)  << std::endl;

    outfile.close();
}

void test_sync_128() {
    cirrus::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    // warm up
    std::cout << "Warming up" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i);
    }

    std::cout << "Warm up done" << std::endl;

    cirrus::RDMAMem mem(d.get(), sizeof(Dummy_128));

    cirrus::Stats stats;
    stats.reserve(MILLION);

    std::cout << "Measuring latencies.." << std::endl;
    for (uint64_t i = 0; i < MILLION; ++i) {
        cirrus::TimerFunction tf;
        store.put(d.get(), sizeof(Dummy_128), i % 1000, &mem);
        uint64_t elapsed_us = tf.getUsElapsed();
        stats.add(elapsed_us);
    }

    uint64_t end;
    std::cout << "Measuring msgs/s.." << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;
    for (; i < 10 * MILLION; ++i) {
        store.put(d.get(), sizeof(Dummy_128), i % 1000, &mem);

        if (i % 100000 == 0) {
            if ((end = start.getUsElapsed()) > MILLION) {
                break;
            }
        }
    }

    std::ofstream outfile;
    outfile.open("throughput_128.log");
    outfile << "throughput 128 test" << std::endl;
    outfile << "count: " << stats.getCount() << std::endl;
    outfile << "msg/s: " << i / (end * 1.0 / MILLION)  << std::endl;
    outfile << "bytes/s: " << (i * sizeof(Dummy_128)) / (end * 1.0 / MILLION)  << std::endl;

    outfile.close();
}

auto main() -> int {
    // test burst of sync writes
    test_sync_128();
    test_sync_4K();
    test_sync_50();
    test_sync_1M();

    return 0;
}