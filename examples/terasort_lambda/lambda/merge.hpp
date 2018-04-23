#ifndef MERGE_HPP_
#define MERGE_HPP_

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include "../S3.hpp"
#include "../config.hpp"

struct merge_lambda {
  std::shared_ptr<Aws::S3::S3Client> _client;
  std::vector<INT_TYPE> _positions;
  std::vector<bool> _input_files_completed;
  INT_TYPE _total_files, _process_index;
  std::atomic<INT_TYPE> _curr_file_num;
  std::unordered_map<INT_TYPE, std::pair<
    INT_TYPE,
    std::shared_ptr<std::string>>> _file_cache;

  std::queue<std::pair<
    INT_TYPE,
    std::shared_ptr<std::string>>> _background_buffer;
  std::vector<std::thread> _background_threads;
  std::mutex _background_lock;
  std::condition_variable _background_cv;

  bool _finished;

  merge_lambda(INT_TYPE p, std::shared_ptr<Aws::S3::S3Client> cl, std::vector<INT_TYPE> pos,
      std::vector<bool> ifc, INT_TYPE n, INT_TYPE c);
  ~merge_lambda();

  void finish();
  void background_buffer_writer();
};

void merger(std::shared_ptr<merge_lambda> ml);

#endif
