#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <cctype>
#include <chrono>
#include <thread>
#include <random>
#include <memory>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
#define SECS_BENCHMARK 5

static const uint64_t KB = 1024;
static const uint64_t MB = 1024 * KB;
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint64_t MILLION = 1000000;
static const uint64_t N_ITER = 1000;

/**
 * This function is run by the threads in the put benchmark.
 */
template <uint64_t SIZE>
void test_put_function(
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> *store,
    int *total, bool *begin_test) {
    struct cirrus::Dummy<SIZE> d(42);
    // wait until signalled to begin
    while (!(*begin_test)) {}
    cirrus::TimerFunction timer;
    for (int loop = 0; 1; loop++) {
        // TODO(Tyler): Set this to put at different IDs once
        // server parallelism is merged
        store->put(0, d);
        if (loop % 1000 == 0 && timer.getSecElapsed() > SECS_BENCHMARK) {
            *total = loop;
            return;
        }
    }
}

/**
 * This function is run by the threads in the get benchmark.
 */
template <uint64_t SIZE>
void test_get_function(
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>> *store,
    int *total, bool *begin_test) {
    // wait until signalled to begin
    while (!(*begin_test)) {}

    cirrus::TimerFunction timer;
    for (int loop = 0; 1; loop++) {
        // TODO(Tyler): Set this to put at different IDs once
        // server parallelism is merged
        store->get(0);
        if (loop % 1000 == 0 && timer.getSecElapsed() > SECS_BENCHMARK) {
            *total = loop;
            return;
        }
    }
}

/**
  * This benchmark aims to find the put bandwidth achieved when it keeps
  * outstanding_target requests in flight.
  */
template <uint64_t SIZE>
void test_put_bandwidth(uint64_t outstanding_target, std::ofstream& outfile) {
    cirrus::TCPClient client;
    // TODO(Tyler): Open additional connections from client once
    // TCP Server parallelism is merged.
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            serializer,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (uint64_t i = 0; i < 1; i++) {
        store.put(i, d);
    }

    bool begin_test = false;
    std::vector<std::thread*> threads(outstanding_target);
    std::vector<int> totals(outstanding_target);

    // launch the threads
    for (unsigned int i = 0; i < outstanding_target; i++) {
        threads[i] = new std::thread(test_put_function<SIZE>, &store,
            &(totals[i]),
            &begin_test);
    }

    cirrus::TimerFunction timer;
    begin_test = true;
    for (unsigned int i = 0; i < outstanding_target; ++i)
        threads[i]->join();

    double secs_elapsed = timer.getSecElapsed();

    int count_completed = 0;
    for (auto& val : totals) {
        count_completed += val;
    }

    double size_completed_MB = 1.0 * count_completed * SIZE / 1024 / 1024;
    double bw_MB = 1.0 * size_completed_MB / secs_elapsed;
    outfile
        << "Put Size (B): "
        << std::left << std::setw(20) << std::setfill(' ')
        << SIZE
        << " # outstanding: "
        << std::left << std::setw(20) << std::setfill(' ')
        << outstanding_target
        << " Bandwidth (MB/s): "
        << std::left << std::setw(20) << std::setfill(' ')
        << bw_MB
        << std::endl;
}

/**
  * This benchmark aims to find the get bandwidth achieved when it keeps
  * outstanding_target requests in flight.
  */
template <uint64_t SIZE>
void test_get_bandwidth(uint64_t outstanding_target, std::ofstream& outfile) {
    cirrus::TCPClient client;
    // TODO(Tyler): Open additional connections from client once
    // TCP Server parallelism is merged.
    cirrus::serializer_simple<cirrus::Dummy<SIZE>> serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            serializer,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (uint64_t i = 0; i < 1; i++) {
        store.put(i, d);
    }

    bool begin_test = false;
    std::vector<std::thread*> threads(outstanding_target);
    std::vector<int> totals(outstanding_target);
    for (unsigned int i = 0; i < outstanding_target; i++) {
        threads[i] = new std::thread(test_get_function<SIZE>, &store,
            &(totals[i]),
            &begin_test);
    }

    cirrus::TimerFunction timer;
    begin_test = true;
    for (unsigned int i = 0; i < outstanding_target; ++i)
        threads[i]->join();

    double secs_elapsed = timer.getSecElapsed();

    int count_completed = 0;
    for (auto& val : totals) {
        count_completed += val;
    }

    double size_completed_MB = 1.0 * count_completed * SIZE / 1024 / 1024;
    double bw_MB = 1.0 * size_completed_MB / secs_elapsed;
    outfile
        << "Put Size (B): "
        << std::left << std::setw(20) << std::setfill(' ')
        << SIZE
        << " # outstanding: "
        << std::left << std::setw(20) << std::setfill(' ')
        << outstanding_target
        << " Bandwidth (MB/s): "
        << std::left << std::setw(20) << std::setfill(' ')
        << bw_MB
        << std::endl;
}

auto main() -> int {
    // test burst of sync writes
    std::ofstream outfile;
    outfile.open("store_bandwidth.log");

    std::vector<uint64_t> outstanding_req = {1, 8, 16};

    for (const auto& outs : outstanding_req) {
        test_put_bandwidth<32>(outs, outfile);
        test_put_bandwidth<1 * KB>(outs, outfile);
        test_put_bandwidth<4 * KB>(outs, outfile);
        // test_put_bandwidth<32 * KB>(outs, outfile);
        // test_put_bandwidth<1 * MB>(outs, outfile);

        test_get_bandwidth<32>(outs, outfile);
        test_get_bandwidth<1 * KB>(outs, outfile);
        test_get_bandwidth<4 * KB>(outs, outfile);
        // test_get_bandwidth<32 * KB>(outs, outfile);
        // test_get_bandwidth<1 * MB>(outs, outfile);
    }

    outfile.close();

    return 0;
}
