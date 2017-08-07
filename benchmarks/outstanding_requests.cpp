#include <stdlib.h>
#include <iostream>
#include <string>
#include <iomanip>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "client/TCPClient.h"

static const uint64_t MILLION = 1000000;
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";

#define KB (1024)
#define MB (KB * KB)
// number of secs each benchmrak is going to run for
#define SECS_BENCHMARK 5

struct BenchmarkStruct {
public:
    explicit BenchmarkStruct(int size) : size(size) {
        std::unique_ptr<char[]> ptr(new char[size]);
        data = std::move(ptr);
        //data = std::make_shared<char[]>(new char[size]);
    }

public:
    std::unique_ptr<char[]> data;
    uint64_t size;
};

std::pair<std::unique_ptr<char[]>, unsigned int>
                         serializer(const BenchmarkStruct& s) {
    std::unique_ptr<char[]> ptr(new char[s.size]);
    std::memcpy(ptr.get(), s.data.get(), s.size);
    return std::make_pair(std::move(ptr), s.size);
}

BenchmarkStruct deserializer(void* data, unsigned int size) {
    char *ptr = reinterpret_cast<char*>(data);
    
    BenchmarkStruct ret(size); // not sure if this size arg is valid here
    std::memcpy(ret.data.get(), ptr, size);
    ret.size = size;
    return ret;
}


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
    cirrus::ostore::FullBladeObjectStoreTempl<BenchmarkStruct>
        store(IP, PORT, &client,
            serializer,
            deserializer);
    
    // we have space for outstanding_target futures
    bool send[outstanding_target] = {0};
    typename cirrus::ObjectStore<BenchmarkStruct>::ObjectStorePutFuture futures[outstanding_target];

    struct BenchmarkStruct d(SIZE);

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
    std::cout 
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
    std::vector<uint64_t> outstanding_req = {1, 2, 5, 10, 20, 30};

    for (const auto& outs : outstanding_req) {
        test_outstanding_requests<1>(outs);
        test_outstanding_requests<1 * KB>(outs);
        test_outstanding_requests<4 * KB>(outs);
        test_outstanding_requests<1 * MB>(outs);
        test_outstanding_requests<10 * MB>(outs);
        test_outstanding_requests<100 * MB>(outs);
    }

    return 0;
}
