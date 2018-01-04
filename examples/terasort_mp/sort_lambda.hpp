#ifndef EXAMPLES_TERASORT_MP_SORT_LAMBDA_HPP_
#define EXAMPLES_TERASORT_MP_SORT_LAMBDA_HPP_

#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include <atomic>
#include <future>
#include <utility>

#include "config_instance.hpp"

#include "object_store/FullBladeObjectStore.h"

namespace cirrus_terasort {

class sort_lambda {
 private:
        std::mutex _read_mutex;
        INT_TYPE _process_index;
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
                <std::string>> _store;

        bool _finished;
        std::atomic<INT_TYPE> _next_index;
        std::vector<INT_TYPE> _index_list;

        std::mutex _stats_lock;
        std::vector<std::pair<long double, long double>>
                _sort_thread_bandwidths, _merge_thread_bandwidths;
        std::vector<std::pair<long double, long double>>
                _get_thread_bandwidths;
        std::vector<std::pair<long double, long double>>
                _sort_get_thread_bandwidths;

 public:
        sort_lambda(std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
                        <std::string>> s, INT_TYPE p);

        std::vector<std::shared_ptr<std::string>> get_records();

        void add_sort_data(long double b, long double t);
        void add_merge_data(long double b, long double t);
        void add_sort_get_data(long double b, long double t);
        void print_avg_stats();
};

void sorter(std::shared_ptr<sort_lambda> sl, std::promise<std::vector<
        std::shared_ptr<std::string>>> p);
void merger(std::shared_ptr<sort_lambda> sl, const std::vector<std::shared_ptr
        <std::string>>& vec1, const std::vector<std::shared_ptr<std::string>>&
        vec2, std::promise<std::vector<std::shared_ptr<std::string>>> p);
}  // namespace cirrus_terasort

#endif  // EXAMPLES_TERASORT_MP_SORT_LAMBDA_HPP_
