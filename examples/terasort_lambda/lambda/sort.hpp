#ifndef SORT_HPP_
#define SORT_HPP_

#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include <atomic>
#include <future>
#include <utility>
#include <queue>

#include "../config.hpp"
#include "../S3.hpp"

#include "object_store/FullBladeObjectStore.h"

class sort_lambda {
  private:
    std::mutex _read_mutex;
    INT_TYPE _process_index;
    std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
      <std::string>> _store;

    INT_TYPE _initial_sort_index;
    bool _finished;
    std::atomic<INT_TYPE> _next_index;
    std::vector<INT_TYPE> _index_list, _initial_index_list;
    std::atomic<INT_TYPE> _next_sort_index;

    std::mutex _stats_lock;
    std::vector<std::pair<long double, long double>>
      _sort_thread_bandwidths, _merge_thread_bandwidths;
    std::vector<std::pair<long double, long double>>
      _get_thread_bandwidths;
    std::vector<std::pair<long double, long double>>
      _sort_get_thread_bandwidths;

    INT_TYPE _background_finished;
    std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;

  public:
    sort_lambda(std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
        <std::string>> s,
        std::shared_ptr<Aws::S3::S3Client> c, INT_TYPE p, std::vector<INT_TYPE> io,
        INT_TYPE nio, INT_TYPE nsio);

    std::tuple<std::vector<char*>, bool, bool>
      get_records();
    std::pair<std::vector<std::shared_ptr<std::string>>, bool> smart_get(
        const std::vector<INT_TYPE>& keys);
    void background_buffer_writer(std::shared_ptr<Aws::S3::S3Client> c);

    INT_TYPE next_sort_index();
    INT_TYPE current_sort_index();
    INT_TYPE initial_sort_index();
    INT_TYPE start_index();
    INT_TYPE next_index();

    void set_finished();
    void finish();
    void add_sort_data(long double b, long double t);
    void add_merge_data(long double b, long double t);
    void add_sort_get_data(long double b, long double t);
    void print_avg_stats();
    const std::chrono::time_point<std::chrono::high_resolution_clock>& start_time();
    const std::vector<INT_TYPE>& index_list();
    const std::vector<INT_TYPE>& initial_index_list();

    std::vector<std::thread> _background_threads;
    std::mutex _background_lock;
    std::condition_variable _background_cv;
    std::queue<std::pair<
        INT_TYPE,
        std::shared_ptr<std::vector<char*>>
        >> _background_buffer;
};

void sorter(std::shared_ptr<sort_lambda> sl,
    std::shared_ptr<Aws::S3::S3Client> c);

#endif

