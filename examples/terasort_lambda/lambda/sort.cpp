#include "sort.hpp"

#include <cmath>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <utility>

#include "timsort.hpp"

sort_lambda::sort_lambda(
    std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
    s, std::shared_ptr<Aws::S3::S3Client> c,
    INT_TYPE p, std::vector<INT_TYPE> io, INT_TYPE nio, INT_TYPE snio) : _process_index(p), _store(s),
    _next_sort_index(_process_index * total_read_keys + snio),
    _background_threads(std::vector<std::thread>{}),
    _background_buffer(std::queue<std::pair<INT_TYPE, std::shared_ptr<
        std::vector<char*>>>>()), _background_finished(0) {
  _start_time = std::chrono::high_resolution_clock::now();
  for (INT_TYPE i = 0; i < rh_nodes; i++)
    _index_list.push_back(total_read_keys + i * rh_node_key_spacing
        + _process_index);
  _initial_index_list = _index_list;
  for (INT_TYPE i = 0; i < rh_nodes; i++)
    _index_list[i] += io[i];

  _initial_sort_index = _process_index * total_read_keys;
  _next_index = 0 + nio;
  _finished = false;

  for (INT_TYPE _ = 0; _ < sort_num_background_threads; _++)
    _background_threads.push_back(std::thread(&sort_lambda::background_buffer_writer, this, c));
}

INT_TYPE sort_lambda::initial_sort_index() {
  return _initial_sort_index;
}

INT_TYPE sort_lambda::current_sort_index() {
  return _next_sort_index;
}

INT_TYPE sort_lambda::next_index() {
  return _next_index;
}

const std::vector<INT_TYPE>& sort_lambda::initial_index_list() {
  return _initial_index_list;
}

const std::chrono::time_point<std::chrono::high_resolution_clock>& sort_lambda::start_time() {
  return _start_time;
}

const std::vector<INT_TYPE>& sort_lambda::index_list() {
  return _index_list;
}

void sort_lambda::set_finished() {
  ++_background_finished;
}

void sort_lambda::finish() {
  _background_finished = sort_threads;
  for (INT_TYPE _ = 0; _ < sort_num_background_threads; _++)
    _background_threads[_].join();
}

void sort_lambda::background_buffer_writer(std::shared_ptr<Aws::S3::S3Client> c) {
  while(_background_finished != sort_threads || _background_buffer.size() > 0) {
    std::unique_lock<std::mutex> lock(_background_lock);
    while(_background_buffer.size() == 0) {
      if(_background_finished == sort_threads) {
        lock.unlock();
        return;
      }
      _background_cv.wait_for(lock, std::chrono::milliseconds(
            rh_hash_background_timeout_ms),
          [this]() { return
          _background_finished == sort_threads ||
          _background_buffer.size() > 0; });
      if(_background_finished == sort_threads && _background_buffer.size() == 0) {
        lock.unlock();
        return;
      }
    }
    std::pair<INT_TYPE, std::shared_ptr<std::vector<char*>>> p =
        _background_buffer.front();
    _background_buffer.pop();

    lock.unlock();
    _background_cv.notify_one();

    std::string key = sort_prefix + std::to_string(p.first);
    std::string res;
    for (const char* s : *p.second)
      res += s, res += '\n';

    s3_put_object(key, *c, s3_bucket_name, res);
    for (char* s : *p.second)
      free(s);
    p.second.reset();
  }
}

INT_TYPE sort_lambda::next_sort_index() {
  return _next_sort_index++;
}

std::pair<std::vector<std::shared_ptr<std::string>>, bool> sort_lambda::smart_get(
    const std::vector<INT_TYPE>& keys) {
  try {
    std::vector<std::string> vals = _store->get_bulk_fast(keys);
    std::vector<std::shared_ptr<std::string>> ret;
    for (const std::string& s : vals)
      ret.push_back(std::make_shared<std::string>(s));
    for (const INT_TYPE& i : keys)
      _store->remove(i);
    return std::make_pair(ret, false);
  }
  catch(...) {  // fall to back serial
    std::vector<std::shared_ptr<std::string>> ret;
    bool retry = true;
    for (const INT_TYPE& k : keys) {
      try {
        std::string chunk = _store->get(k);
        ret.push_back(
            std::make_shared<std::string>(chunk));
        _store->remove(k);
        if(chunk == sentinel) {
          retry = false;
          break;
        }
      }
      catch (...) {
        retry = true;
        break;
      }
    }
    return std::make_pair(ret, retry);
  }
}

std::tuple<std::vector<char*>, bool, bool>
sort_lambda::get_records() {
  std::vector<char*> ret;
  bool wait = false;

  _read_mutex.lock();

  auto get_start = std::chrono::high_resolution_clock::now();
  for (INT_TYPE n = _next_index; n < rh_nodes &&
      !_finished; n++) {
    std::vector<INT_TYPE> keys;
    for(INT_TYPE j = 0; j < sort_bulk_transfer;
        j++) {
      keys.push_back(_index_list[n] +
          j * sort_nodes);
      //std::cout << keys[keys.size() - 1] << std::endl;
    }
    const std::pair<std::vector<std::shared_ptr
      <std::string>>, bool>& p = smart_get(keys);
    const std::vector<std::shared_ptr<std::string>>& chunks =
      std::move(p.first);
    const bool& retry = p.second;
    wait = retry;

    //std::cout << "chunk size " << _next_index << " " << chunks.size() << std::endl;
    _index_list[n] += sort_nodes * chunks.size();
    if (chunks.size() == 0 && !retry) {
      _finished = ++_next_index >=
        rh_nodes;
    } else {
      for(const std::shared_ptr<std::string>& chunk_ptr :
          chunks) {
        std::string chunk = *chunk_ptr;
        if(chunk == sentinel) {
          _finished = ++_next_index >=
            rh_nodes;
          break;
        }
        for (INT_TYPE i = 0; i <
            cirrus_read_chunk_size && 
            (i + 1) * (record_size
              + 1) <= chunk.size(); i++) {
          char* temp = (char*) malloc(
              record_size + 1);
          memcpy(temp, chunk.c_str() + (i *
                (record_size + 1)),
              record_size);
          temp[record_size] = 0;
          ret.push_back(temp);
        }
      }
      break;
    }
  }
  auto get_end = std::chrono::high_resolution_clock::now();

  long double bytes = ret.size() * (record_size + 1),
       time = std::chrono::duration_cast<std::chrono::microseconds>
         (get_end - get_start).count();
  _stats_lock.lock();
  _get_thread_bandwidths.push_back(std::make_pair(bytes, time));
  _stats_lock.unlock();

  _read_mutex.unlock();
  return std::make_tuple(ret, _finished, wait);
}

void sort_lambda::add_sort_data(long double b, long double t) {
  _stats_lock.lock();

  _sort_thread_bandwidths.push_back(std::make_pair(b, t));

  _stats_lock.unlock();
}

void sort_lambda::add_merge_data(long double b, long double t) {
  _stats_lock.lock();

  _merge_thread_bandwidths.push_back(std::make_pair(b, t));

  _stats_lock.unlock();
}

void sort_lambda::add_sort_get_data(long double b, long double t) {
  _stats_lock.lock();

  _sort_get_thread_bandwidths.push_back(std::make_pair(b, t));

  _stats_lock.unlock();
}

void sort_lambda::print_avg_stats() {
  long double  sort_avg = 0, merge_avg = 0, sort_get_avg = 0;
  for (std::pair<long double, long double> p : _sort_thread_bandwidths)
    sort_avg += (p.first / 1e6) / (p.second / 1e3);

  for (std::pair<long double, long double> p :
      _sort_get_thread_bandwidths)
    sort_get_avg += (p.first / 1e6) / (p.second / 1e3);

  std::string to_print = "Sorting/merging process " +
    std::to_string(_process_index) + " total get rate: " +
    std::to_string(sort_get_avg) + " MB/s\n";
  to_print += "Sorting/merging process " + std::to_string(_process_index)
    + " average get rate: " +
    std::to_string(sort_get_avg /
        _sort_get_thread_bandwidths.size())
    + " MB/s\n";
  to_print += "Sorting/merging process " + std::to_string(_process_index)
    + " total sort rate: " + std::to_string(sort_avg) + " MB/s\n";
  to_print += "Sorting/merging process " + std::to_string(_process_index)
    + " average sort rate: " +
    std::to_string(sort_avg / _sort_thread_bandwidths.size())
    + " MB/s\n";

  std::cout << to_print;
}

void sorter(std::shared_ptr<sort_lambda> sl,
    std::shared_ptr<Aws::S3::S3Client> c) {
  std::vector<char*> curr;
  std::shared_ptr<std::vector<char*>> recs = std::make_shared<std::vector<char*>>();
  auto sort_get_start = std::chrono::high_resolution_clock::now();
  INT_TYPE target_size = std::ceil(
      (long double) num_records /
      (long double) sort_nodes /
      (long double) sort_threads);
  bool retry = true;
  while (retry && recs->size() < 2 * target_size) {
    const auto& t = sl->get_records();
    curr = std::get<0>(t), retry = !std::get<1>(t);
    bool wait = std::get<2>(t);
    INT_TYPE pos = 0;
    for (; pos < curr.size() && recs->size() < s3_read_chunk_size; pos++)
      recs->push_back(curr[pos]);
    if(wait || recs->size() == s3_read_chunk_size) {
      auto sort_start = std::chrono::high_resolution_clock::now();
      gfx::timsort(recs->begin(), recs->end(),
          [](const char* a, const char* b) {
          return strcmp(a, b) < 0;
          });
      auto sort_end = std::chrono::high_resolution_clock::now();

      if (recs->size() == s3_read_chunk_size) {
        long double time =
          std::chrono::duration_cast<std::chrono::milliseconds>
          (sort_end - sort_start).count(),
          bytes = recs->size() * record_size;
        sl->add_sort_data(bytes, time);
      }
      std::this_thread::sleep_for(
          std::chrono::milliseconds(
            sort_transfer_wait_ms
            ));
    }
    if (recs->size() == s3_read_chunk_size) {
      std::unique_lock<std::mutex> lock(sl->_background_lock);
      sl->_background_buffer.push(std::make_pair(
            sl->next_sort_index(),
            recs));
      lock.unlock();
      sl->_background_cv.notify_all();
      recs = std::make_shared<std::vector<char*>>();
    }
    for (; pos < curr.size() && recs->size() < s3_read_chunk_size; pos++)
      recs->push_back(curr[pos]);

    auto curr_iter_end = std::chrono::high_resolution_clock::now();
    INT_TYPE time_from_start = std::chrono::duration_cast<std::chrono::seconds>(
        curr_iter_end - sl->start_time()).count();
    if(time_from_start >= sort_timeout_threshold_s)
      break;
  }
  auto sort_get_end = std::chrono::high_resolution_clock::now();
  long double get_time =
    std::chrono::duration_cast<std::chrono::milliseconds>
    (sort_get_end - sort_get_start).count(),
    get_bytes = recs->size() * record_size;
  sl->add_sort_get_data(get_bytes, get_time);

  auto sort_start = std::chrono::high_resolution_clock::now();
  gfx::timsort(recs->begin(), recs->end(),
      [](const char* a, const char* b) {
      return strcmp(a, b) < 0;
      });
  auto sort_end = std::chrono::high_resolution_clock::now();

  long double time =
    std::chrono::duration_cast<std::chrono::milliseconds>
    (sort_end - sort_start).count(),
    bytes = recs->size() * record_size;
  sl->add_sort_data(bytes, time);

  std::unique_lock<std::mutex> lock(sl->_background_lock);
  sl->_background_buffer.push(std::make_pair(
        sl->next_sort_index(),
        recs));
  lock.unlock();
  sl->_background_cv.notify_all();
  sl->set_finished();
}

