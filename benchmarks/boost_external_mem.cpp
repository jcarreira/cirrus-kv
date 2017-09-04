#include <iostream>
#include <cstdlib>

#include "utils/CirrusTime.h"
#include "utils/Stats.h"

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/managed_external_buffer.hpp>

static const uint64_t size = (1024*1024*1024); // 1GB

/**
  * This code benchmarks the time it takes to allocate memory
  * using boost vs using libc
  */
void benchmark(bool deallocate) {
    char* data = new char[size];
    boost::interprocess::managed_external_buffer allocator(
                boost::interprocess::create_only_t(), data, size);

    cirrus::Stats stats_boost;
    cirrus::Stats stats_libc;

    for (int i = 0; i < 100; ++i) {
        uint64_t rand_size = rand() % 1000000;

        cirrus::TimerFunction tf;
        char* ptr = (char*)allocator.allocate(rand_size);
        auto boost_elapsed = tf.getUsElapsed();
        stats_boost.add(boost_elapsed);

        if (deallocate)
            allocator.deallocate(ptr);

        tf.reset();
        ptr = new char[rand_size];
        auto libc_elapsed = tf.getUsElapsed();
        stats_libc.add(libc_elapsed);
        
        if (deallocate)
            delete[] ptr;
        
        std::cout << "boost: " << boost_elapsed 
            << " libc: " << libc_elapsed
            << " size: " << rand_size
            << std::endl;
    }

    std::cout << "Boost stats" << std::endl;
    stats_boost.print();
    std::cout << "Libc stats" << std::endl;
    stats_libc.print();

    delete[] data;
}

int main() {

    benchmark(false);
    benchmark(true);

    return 0;
}
