#include "hash.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <string>

hash_lambda::hash_lambda(
  std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
  <std::string>> store,
  INT_TYPE p, INT_TYPE s, INT_TYPE e, const std::vector<INT_TYPE>& c) : _start(s),
  _end(e), _process_index(p), _counter_list(std::vector<INT_TYPE>{}),
  _background_buffer(std::queue<std::pair<
      std::shared_ptr<std::vector<INT_TYPE>>,
      std::shared_ptr<std::vector<std::string>>
      >>{}), _finished(false),
  _write_buffer(std::vector<std::pair<std::shared_ptr<std::vector<INT_TYPE>>,
      std::shared_ptr<std::vector<std::string>>>>{}),
  _background_threads(std::vector<std::thread>{}) {
  _start_time = std::chrono::high_resolution_clock::now();
  _read_counter = _start;
  _total_size = 0;

  for (INT_TYPE i = 0; i < sort_nodes; i++) {
    _counter_list.push_back(c[i]);
    _write_buffer.push_back(std::make_pair(
        std::make_shared<std::vector<INT_TYPE>>(),
        std::make_shared<std::vector<std::string>>()));
    _write_mutexes.push_back(std::make_shared<std::mutex>());
    _write_buffer_sizes.push_back(0);
  }

  for(INT_TYPE _ = 0; _ < rh_hash_num_background_threads;
      _++)
    _background_threads.push_back(std::thread(
          &hash_lambda::background_buffer_writer, this, store));
}

void hash_lambda::background_buffer_writer(
    std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
    <std::string>> store) {
  while(!_finished || _background_buffer.size() > 0) {
    std::unique_lock<std::mutex> lock(_background_lock);
    while(_background_buffer.size() == 0) {
      if(_finished) {
        lock.unlock();
        return;
      }
      _background_cv.wait_for(lock, std::chrono::milliseconds(
            rh_hash_background_timeout_ms),
          [this]() { return
          _finished ||
          _background_buffer.size() > 0; });
      if(_finished && _background_buffer.size() == 0) {
        lock.unlock();
        return;
      }
    }
    std::pair<std::shared_ptr<std::vector<INT_TYPE>>,
      std::shared_ptr<std::vector<std::string>>> p =
        _background_buffer.front();
    _background_buffer.pop();

    lock.unlock();
    _background_cv.notify_one();

    store->put_bulk_fast(*p.first, *p.second);

    p.first.reset();
    p.second.reset();
  }
}

void hash_lambda::finish(
    std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
    store) {
  for (INT_TYPE k = 0; k < sort_nodes; k++) {
    INT_TYPE start_offset = _process_index * rh_node_key_spacing +
      total_read_keys + k;
    if(_write_buffer[k].first->size() != _write_buffer[k].second->size()) {
      INT_TYPE key_offset = _counter_list[k]++ * sort_nodes;
      _write_buffer[k].first->push_back(start_offset + key_offset);
    }
    if (_read_counter >= _end) {
      INT_TYPE key_offset2 = _counter_list[k]++ *
        sort_nodes;
      std::cout << "Sentinel written " << start_offset + key_offset2 << std::endl;
      _write_buffer[k].first->push_back(start_offset + key_offset2);
      _write_buffer[k].second->push_back(sentinel);
    }
    store->put_bulk_fast(*_write_buffer[k].first, *_write_buffer[k].second);
    _write_buffer[k].first.reset();
    _write_buffer[k].second.reset();
  }

  _finished = true;
  for(INT_TYPE _ = 0; _ < rh_hash_num_background_threads;
      _++)
    _background_threads[_].join();
}

void hash_lambda::write(
    std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
      store,
    INT_TYPE k, const std::string& v, INT_TYPE substr_start,
    INT_TYPE substr_len) {
  if (k < 0 || k > sort_nodes)
    throw std::runtime_error("hashing logic failed.");

  _write_mutexes[k]->lock();

  if (_write_buffer_sizes[k] >= cirrus_read_chunk_size) {
    INT_TYPE start_offset = _process_index * rh_node_key_spacing +
      total_read_keys + k;
    INT_TYPE key_offset = _counter_list[k]++ *
      sort_nodes;
    _write_buffer[k].first->push_back(start_offset + key_offset);
    if (_write_buffer[k].second->size() >=
        rh_hash_bulk_put || 
        std::accumulate(_write_buffer.begin(), _write_buffer.end(), 0,
          [](auto& i, const auto& p) {
            for(auto& s : *p.second) i += s.size() / (record_size + 1);
            return i;
          }) / cirrus_read_chunk_size >= rh_cumulative_size_threshold) {
      std::unique_lock<std::mutex> lock(_background_lock);
      _background_buffer.push(std::make_pair(
            _write_buffer[k].first,
            _write_buffer[k].second));
      lock.unlock();
      _background_cv.notify_all();

      _write_buffer[k].first = std::make_shared<std::vector<INT_TYPE>>();
      _write_buffer[k].second = std::make_shared<std::vector<std::string>>();
      while (_background_buffer.size() >= rh_background_buffer_limit)
        std::this_thread::sleep_for(std::chrono::milliseconds(
            rh_background_buffer_limit_timeout_ms));
    }

    _write_buffer_sizes[k] = 0;
  }
  if(_write_buffer[k].first->size() == _write_buffer[k].second->size())
    _write_buffer[k].second->push_back("");
  (*_write_buffer[k].second)[_write_buffer[k].first->size()].append(v, substr_start, substr_len);
  (*_write_buffer[k].second)[_write_buffer[k].first->size()] += '\n';
  _write_buffer_sizes[k]++;
  
  _write_mutexes[k]->unlock();
}

INT_TYPE hash_lambda::next_counter() {
  return (_read_counter += rh_hash_bulk_get)
    - rh_hash_bulk_get;
}

INT_TYPE hash_lambda::start() {
  return _start;
}

INT_TYPE hash_lambda::end() {
  return _end;
}

INT_TYPE hash_lambda::current() {
  return _read_counter;
}

const std::chrono::time_point<std::chrono::high_resolution_clock>& hash_lambda::start_time() {
  return _start_time;
}

const std::vector<INT_TYPE>& hash_lambda::counter_list() {
  return _counter_list;
}

void hash_lambda::add_data(long double b, long double t) {
  _stats_lock.lock();

  _thread_bandwidths.push_back(std::make_pair(b, t));

  _stats_lock.unlock();
}

void hash_lambda::add_get_data(long double b, long double t) {
  _stats_lock.lock();

  _get_bandwidths.push_back(std::make_pair(b, t));

  _stats_lock.unlock();
}

void hash_lambda::print_avg_stats() {
  long double avg = 0, get_avg = 0;
  for (std::pair<long double, long double> p : _thread_bandwidths)
    avg += (p.first / 1e6) / (p.second / 1e6);

  for (std::pair<long double, long double> p : _get_bandwidths)
    get_avg += (p.first / 1e6) / (p.second / 1e6);

  std::string to_print = "Hashing process " +
    std::to_string(_process_index) + " total hash rate: " +
    std::to_string(avg) + " MB/s\n";
  to_print += "Hashing process " + std::to_string(_process_index) +
    " average hash rate: " +
    std::to_string(avg / _thread_bandwidths.size()) + " MB/s\n";
  to_print += "Hashing process " + std::to_string(_process_index) +
    " total get rate: " + std::to_string(get_avg) + " MB/s\n";
  to_print += "Hashing process " + std::to_string(_process_index) +
    " average get rate: " +
    std::to_string(get_avg / _get_bandwidths.size()) + " MB/s\n";

  std::cout << to_print;
}

static inline INT_TYPE hash_helper(const std::string& s, INT_TYPE start) {
  INT_TYPE ret = 0;
  for (INT_TYPE i = 0; i < rh_hash_bytes; i++)
    ret = chars * ret +
      key_map[s[start + i]];
  return ret;
}

bool hasher_smart_get(std::shared_ptr<hash_lambda> hl,
    std::shared_ptr<Aws::S3::S3Client> c,
    const INT_TYPE& start,
    const INT_TYPE& end, std::string* data) {
  for (INT_TYPE i = start; i <= end; i++)
    data[i - start] = s3_get_object(read_prefix + std::to_string(i), *c,
      s3_bucket_name);
  return true;
}

void hasher(std::shared_ptr<hash_lambda> hl,
    std::shared_ptr<Aws::S3::S3Client> c,
    std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
    store) {
  long double range = (long double) std::pow(chars,
      rh_hash_bytes) /
    (long double) sort_nodes;

  INT_TYPE curr, counter;
  std::string* data_array = new std::string
    [rh_hash_bulk_get];
  while ((curr = hl->next_counter()) < hl->end()) {
    auto get_start = std::chrono::high_resolution_clock::now();
    INT_TYPE start = curr,
              end = std::min(
                  curr + rh_hash_bulk_get,
                  hl->end()) - 1;
    while(!hasher_smart_get(hl, c, start, end, data_array))
      std::this_thread::sleep_for(std::chrono::milliseconds(
            rh_hash_get_retry_ms));
    auto get_end = std::chrono::high_resolution_clock::now();
    long double get_time = std::chrono::duration_cast
      <std::chrono::microseconds>(get_end - get_start)
    .count();
    long double bytes = 0;

    auto hash_start = std::chrono::high_resolution_clock::now();
    for (INT_TYPE _ = 0; _ < end - start + 1; _++) {
      const std::string& chunk = data_array[_];
      bytes += chunk.size();
      for (INT_TYPE i = 0;
        i < s3_read_chunk_size &&
        (i + 1) * (record_size + 1) <=
        chunk.size(); i++, counter++) {
        INT_TYPE h = (long double) hash_helper(chunk,
            i * (record_size + 1))
          / range;
        hl->write(store, h, chunk,
            i * (record_size + 1),
            record_size);
      }
    }
    auto hash_end = std::chrono::high_resolution_clock::now();
    long double hash_time = std::chrono::duration_cast
      <std::chrono::microseconds>
      (hash_end - hash_start).count();
    hl->add_data(bytes, hash_time);
    store->removeBulk(start, end);
    hl->add_get_data(bytes, get_time);
    auto curr_iter_end = std::chrono::high_resolution_clock::now();
    INT_TYPE time_so_far = std::chrono::duration_cast<std::chrono::seconds>(
        curr_iter_end - hl->start_time()).count();
    if (time_so_far >= rh_timeout_threshold_s)
      break;
  }

  delete[] data_array;
}

