#ifndef HASH_HPP
#define HASH_HPP

#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <fstream>
#include <utility>
#include <map>
#include <vector>
#include <queue>
#include <condition_variable>

#include "../S3.hpp"
#include "../config.hpp"
#include "object_store/FullBladeObjectStore.h"

class hash_lambda {
  private:
    std::vector<std::pair<
      std::shared_ptr<std::vector<INT_TYPE>>,
      std::shared_ptr<std::vector<std::string>>
        >> _write_buffer;
    std::vector<INT_TYPE> _write_buffer_sizes;
    INT_TYPE _total_size;
    std::vector<std::shared_ptr<std::mutex>> _write_mutexes;
    bool _finished;

    std::vector<INT_TYPE> _counter_list;
    std::atomic<INT_TYPE> _read_counter;
    INT_TYPE _start, _end, _process_index;

    std::queue<std::pair<
      std::shared_ptr<std::vector<INT_TYPE>>,
      std::shared_ptr<std::vector<std::string>>
        >> _background_buffer;
    std::vector<std::thread> _background_threads;
    std::mutex _background_lock;
    std::condition_variable _background_cv;

    std::mutex _stats_lock;
    std::vector<std::pair<long double, long double>> _thread_bandwidths;
    std::vector<std::pair<long double, long double>> _get_bandwidths;
    std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;

  public:
    hash_lambda(
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
        <std::string>> store,
        INT_TYPE p, INT_TYPE s, INT_TYPE e, const std::vector<INT_TYPE>& c);

    void write(std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
        <std::string>> store, INT_TYPE k, const std::string& v,
        INT_TYPE substr_start, INT_TYPE substr_len);
    void finish(std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
        <std::string>> store);

    INT_TYPE next_counter();
    INT_TYPE start();
    INT_TYPE end();
    INT_TYPE current();
    const std::vector<INT_TYPE>& counter_list();

    void add_data(long double b, long double t);
    void add_get_data(long double b, long double t);
    void print_avg_stats();

    void background_buffer_writer(
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
        <std::string>> store);
    const std::chrono::time_point<std::chrono::high_resolution_clock>& start_time();
};

bool hasher_smart_get(std::shared_ptr<hash_lambda> hl,
    std::shared_ptr<Aws::S3::S3Client> c,
    const INT_TYPE& start,
    const INT_TYPE& end, std::string* data);
void hasher(std::shared_ptr<hash_lambda> hl,
    std::shared_ptr<Aws::S3::S3Client> c,
    std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
    <std::string>> store);

#endif
