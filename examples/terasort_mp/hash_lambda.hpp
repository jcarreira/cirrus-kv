#ifndef EXAMPLES_TERASORT_MP_HASH_LAMBDA_HPP_
#define EXAMPLES_TERASORT_MP_HASH_LAMBDA_HPP_

#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <fstream>
#include <utility>
#include <map>
#include <vector>

#include "config_instance.hpp"

#include "object_store/FullBladeObjectStore.h"

namespace cirrus_terasort {

class hash_lambda {
 private:
        std::vector<std::string> _write_buffer;
        std::vector<INT_TYPE> _write_buffer_sizes;
        std::vector<std::shared_ptr<std::mutex>> _write_mutexes;

        std::vector<INT_TYPE> _counter_list;
        std::atomic<INT_TYPE> _read_counter;
        INT_TYPE _start, _end, _process_index;

        std::mutex _stats_lock;
        std::vector<std::pair<long double, long double>> _thread_bandwidths;
        std::vector<std::pair<long double, long double>> _get_bandwidths;

 public:
        hash_lambda(INT_TYPE p, INT_TYPE s, INT_TYPE e);

        void write(std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
                        <std::string>> store, INT_TYPE k, const std::string& v,
                        INT_TYPE substr_start, INT_TYPE substr_len);
        void finish(std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
                        <std::string>> store);

        INT_TYPE next_counter();
        INT_TYPE start();
        INT_TYPE end();

        void add_data(long double b, long double t);
        void add_get_data(long double b, long double t);
        void print_avg_stats();
};

void hasher(std::shared_ptr<hash_lambda> hl,
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
                <std::string>> store);
}  // namespace cirrus_terasort

#endif  // EXAMPLES_TERASORT_MP_HASH_LAMBDA_HPP_
