/* Copyright Joao Carreira 2017 */

/**
  * Here we test the use of Cirrus on a distributed application
  * built on top of MPI
  *
  */

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
#include <unistd.h>
#include <mpi.h>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/Time.h"
#include "src/utils/Stats.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1;

struct Dummy {
    char data[SIZE];
    int id;
};

//#define CHECK_RESULTS

void test_sync() {
    sirius::ostore::FullBladeObjectStoreTempl<> store(IP, PORT);

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;

    try {
        store.put(d.get(), sizeof(Dummy), 1);
    } catch(...) {
        throw std::runtime_error("Error inserting");
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

void test_sync(int N) {
    sirius::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);
    sirius::Stats stats;

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    d->id = 42;
    Dummy* d2 = reinterpret_cast<Dummy*>(::operator new(sizeof(Dummy)));

    // warm up
    for (int i = 0; i < 100; ++i) {
        store.put(d.get(), sizeof(Dummy), 1);
    }

    // real benchmark
    for (int i = 0; i < N; ++i) {
        sirius::TimerFunction tf("", false);
        store.put(d.get(), sizeof(Dummy), 1);
#ifdef CHECK_RESULTS
        store.get(1, d2);
        if (reinterpret_cast<Dummy*>(d2)->id != 42) {
            throw std::runtime_error("Wrong value");
        }
#endif

        uint64_t elapsed = tf.getUsElapsed();
        stats.add(elapsed);
    }

    std::cout << "avg: " << stats.avg() << std::endl;
    std::cout << "sd: " << stats.sd() << std::endl;
    std::cout << "99%: " << stats.getPercentile(0.99) << std::endl;
}

void test_async_N(int N) {
    sirius::ostore::FullBladeObjectStoreTempl<Dummy> store(IP, PORT);
    sirius::Stats stats;

    std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
    sirius::TimerFunction tfs[N];
    d->id = 42;

    std::function<bool(bool)> futures[N];
    
    // warm up
    for (int i = 0; i < N; ++i) {
        store.put(d.get(), sizeof(Dummy), 1);
    }

    std::cout << "Warm up done" << std::endl;

    for (int i = 0; i < N; ++i) {
        tfs[i].reset();
        std::cout << "put" << std::endl;
        futures[i] = store.put_async(d.get(), sizeof(Dummy), 1);
    }

    bool done[N];
    std::memset(done, 0, sizeof(done));
    int total_done = 0;

    while (total_done != N) {
        for (int i = 0; i < N;++i) {
            if (!done[i]) {
                bool ret = futures[i](false);
                if (ret) {
                    done[i] = true;
                    total_done++;
                    auto elapsed = tfs[i].getUsElapsed();
                    stats.add(elapsed);
                }
            }
        }
    }

    std::cout << "count: " << stats.getCount() << std::endl;
    std::cout << "avg: " << stats.avg() << std::endl;
    std::cout << "sd: " << stats.sd() << std::endl;
    std::cout << "99%: " << stats.getPercentile(0.99) << std::endl;
}

inline
void init_mpi(int argc, char**argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        std::cerr
            << "MPI implementation does not support multiple threads"
            << std::endl;
//#MPI_Abort(MPI_COMM_WORLD, 1); 
    }   
}

int main(int argc, char** argv) {
    int rank, nprocs;

    init_mpi(argc, argv);
    int err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    err = MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    //check_mpi_error(err);

    char name[200];
    gethostname(name, 200);
    std::cout << "MPI test running on hostname: " << name << std::endl;

    test_sync();

    MPI_Finalize();

    std::cout << "Test successful" << std::endl;

    return 0;
}

