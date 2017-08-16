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
static const uint64_t MILLION = 1000;
static const uint64_t N_ITER = 1000;

/**
  * This benchmark has two aims
  * 1. find the distribution of latencies for synchronous puts
  * 2. measure the throughput in terms of messages sent / second
  */
template <uint64_t SIZE>
void test_put_bandwidth(uint64_t outstanding_target, std::ofstream& outfile) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (uint64_t i = 0; i < N_ITER; ++i) {
        store.put(i, d);
    }

    // we have space for outstanding_target futures
    bool send[outstanding_target] = {0};
    typename cirrus::ObjectStore<cirrus::Dummy<SIZE>>::ObjectStorePutFuture
        futures[outstanding_target];

    uint64_t count_completed = 0;
    cirrus::TimerFunction timer;
    for (uint64_t loop = 0; 1; loop++) {
        // an entry in the futures array is either:
        // 1. initialized by default (begin of execution)
        // 2. completed (try_wait returns true)
        // 3. outstanding (try_wait returns false)
        for (unsigned int i = 0; i < outstanding_target; i++) {
            if (!send[i]) {
                send[i] = true;
                futures[i] = store.put_async(0, d);
            } else if (futures[i].try_wait()) {
                futures[i] = store.put_async(0, d);
                count_completed++;
            }

            if (loop % MILLION == 0) {
                if (timer.getSecElapsed() > SECS_BENCHMARK) {
                    goto end_benchmark;
                }
            }
        }
    }
    end_benchmark:
    double size_completed_MB = 1.0 * count_completed * SIZE / 1024 / 1024;
    double secs_elapsed = timer.getSecElapsed();
    double bw_MB = 1.0 * size_completed_MB / secs_elapsed;
    outfile
        << "Size (B): "
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
  * This benchmark has two aims
  * 1. find the distribution of latencies for synchronous puts
  * 2. measure the throughput in terms of messages sent / second
  */
template <uint64_t SIZE>
void test_get_bandwidth(uint64_t outstanding_target, std::ofstream& outfile) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (uint64_t i = 0; i < N_ITER; ++i) {
        store.put(i, d);
    }

    // we have space for outstanding_target futures
    bool send[outstanding_target] = {0};
    typename cirrus::ObjectStore<cirrus::Dummy<SIZE>>::ObjectStoreGetFuture
        futures[outstanding_target];

    uint64_t count_completed = 0;
    cirrus::TimerFunction timer;
    for (uint64_t loop = 0; 1; ++loop) {
        // an entry in the futures array is either:
        // 1. initialized by default (begin of execution)
        // 2. completed (try_wait returns true)
        // 3. outstanding (try_wait returns false)
        for (unsigned int i = 0; i < outstanding_target; ++i) {
            if (!send[i]) {
                send[i] = true;
                futures[i] = store.get_async(0);
            } else if (futures[i].try_wait()) {
                futures[i] = store.get_async(0);
                count_completed++;
            }

            if (loop % MILLION == 0) {
                if (timer.getSecElapsed() > SECS_BENCHMARK) {
                    goto end_benchmark;
                }
            }
        }
    }
end_benchmark:
    double size_completed_MB = 1.0 * count_completed * SIZE / 1024 / 1024;
    double secs_elapsed = timer.getSecElapsed();
    double bw_MB = 1.0 * size_completed_MB / secs_elapsed;
    outfile
        << "Size (B): "
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
        test_put_bandwidth<32 * KB>(outs, outfile);
        test_put_bandwidth<1 * MB>(outs, outfile);

        test_get_bandwidth<32>(outs, outfile);
        test_get_bandwidth<1 * KB>(outs, outfile);
        test_get_bandwidth<4 * KB>(outs, outfile);
        test_get_bandwidth<32 * KB>(outs, outfile);
        test_get_bandwidth<1 * MB>(outs, outfile);
    }

    outfile.close();

    return 0;
}
