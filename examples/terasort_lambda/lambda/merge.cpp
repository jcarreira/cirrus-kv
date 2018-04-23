#include "merge.hpp"
#include "partial_get.hpp"

merge_lambda::merge_lambda(INT_TYPE p, std::shared_ptr<Aws::S3::S3Client> cl,
    std::vector<INT_TYPE> pos, std::vector<bool> ifc, INT_TYPE n, INT_TYPE c) :
  _process_index(p), _client(cl), _positions(pos), _total_files(n), _curr_file_num(c),
  _file_cache(std::unordered_map<INT_TYPE, std::pair<INT_TYPE,
      std::shared_ptr<std::string>>>{}), _input_files_completed(ifc),
  _background_buffer(std::queue<std::pair<INT_TYPE, std::shared_ptr<std::string>>>{}),
  _background_threads(std::vector<std::thread>{}), _finished(false) {

  for(INT_TYPE _ = 0; _ < merge_num_background_threads; _++)
    _background_threads.push_back(std::thread(&merge_lambda::background_buffer_writer, this));
}

merge_lambda::~merge_lambda() {

}

void merge_lambda::finish() {
  _finished = true;
  for(INT_TYPE _ = 0; _ < merge_num_background_threads; _++)
    _background_threads[_].join();
}

void merge_lambda::background_buffer_writer() {
  while(!_finished || _background_buffer.size() > 0) {
    std::unique_lock<std::mutex> lock(_background_lock);
    while(_background_buffer.size() == 0) {
      if(_finished) {
        lock.unlock();
        return;
      }
      _background_cv.wait_for(lock, std::chrono::milliseconds(
            rh_hash_background_timeout_ms),
          [this]() {
            return _finished || _background_buffer.size() > 0;
          });
      if(_finished && _background_buffer.size() == 0) {
        lock.unlock();
        return;
      }
    }

    std::pair<INT_TYPE, std::shared_ptr<std::string>> p = _background_buffer.front();
    _background_buffer.pop();

    lock.unlock();
    _background_cv.notify_one();

    s3_put_object(merge_prefix + std::to_string(p.first), *_client, s3_bucket_name, *p.second);

    p.second.reset();
  }
}

void merger(std::shared_ptr<merge_lambda> ml) {
  std::shared_ptr<std::string> running = std::make_shared<std::string>();
  while(std::any_of(ml->_input_files_completed.begin(), ml->_input_files_completed.end(),
        [](const auto& c) { return !c; })) {

    std::string curr_min;
    INT_TYPE min_index = -1;

    for(INT_TYPE i = 0; i < ml->_total_files; i++) {
      if(ml->_input_files_completed[i]) continue;

      INT_TYPE curr_pos = ml->_positions[i];

      auto iter = ml->_file_cache.find(i);
      bool get_from_server;
      if(iter == ml->_file_cache.end()) get_from_server = true;
      else {
        INT_TYPE start_pos = iter->second.first;
        std::shared_ptr<std::string> data = iter->second.second;
        if(start_pos <= curr_pos && curr_pos + (record_size + 1) <= start_pos + data->size())
          get_from_server = true;
        else
          get_from_server = false;
      }

      if(get_from_server) {
        if(ml->_file_cache.size() > merge_file_cache_size)
          ml->_file_cache.erase(ml->_file_cache.begin());

        std::shared_ptr<std::string> pg = std::make_shared<std::string>(
            partial_get(sort_prefix + std::to_string(ml->_process_index * total_read_keys + i),
              curr_pos, curr_pos + merge_file_cache_file_size));
        if(pg->length() == 0) ml->_input_files_completed[i] = true;
        ml->_file_cache[i] = std::make_pair(curr_pos, pg);
        iter = ml->_file_cache.find(i);
      }

      if(ml->_input_files_completed[i]) continue;

      INT_TYPE start_pos = iter->second.first;
      std::shared_ptr<std::string> data = iter->second.second;
      INT_TYPE start_index = curr_pos - start_pos;

      if(curr_min.length() == 0)
        curr_min = std::string(data->begin() + start_pos,
            data->begin() + start_pos + record_size + 1), min_index = i;
      else if(curr_min.compare(start_pos, record_size + 1, *data))
        curr_min = std::string(data->begin() + start_pos,
            data->begin() + start_pos + record_size + 1), min_index = i;
    }

    if(min_index != -1) ml->_positions[min_index] += record_size + 1;
    else break;

    *running += curr_min;
    while(ml->_background_buffer.size() >= merge_background_buffer_limit)
      std::this_thread::sleep_for(std::chrono::milliseconds(
            merge_background_buffer_limit_timeout_ms));

    if(running->length() >= s3_read_chunk_size ||
        std::all_of(ml->_input_files_completed.begin(), ml->_input_files_completed.end(),
          [](const auto& c) { return c; })) {
      std::unique_lock<std::mutex> lock(ml->_background_lock);
      ml->_background_buffer.push(make_pair(ml->_curr_file_num++, running));
      running = std::make_shared<std::string>();
    }
  }
}
