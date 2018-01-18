#include "S3SparseIterator.h"
#include "Utils.h"
#include <unistd.h>
#include <vector>
#include <iostream>
#include <CircularBuffer.h>

#include <pthread.h>
#include <semaphore.h>


sem_t semaphore;

int str_version = 0;
std::map<int, std::string> list_strings; // strings from s3

CircularBuffer<std::pair<const void*, int>> minibatches_list(100000);

// s3_cad_size nmber of samples times features per sample
S3SparseIterator::S3SparseIterator(
        uint64_t left_id, uint64_t right_id,
        const Configuration& c,
        uint64_t s3_rows,
        uint64_t minibatch_rows) :
    left_id(left_id), right_id(right_id),
    conf(c), s3_rows(s3_rows),
    minibatch_rows(minibatch_rows) {
      
  std::cout << "Creating S3SparseIterator"
    << " left_id: " << left_id
    << " right_id: " << right_id
    << std::endl;

  // initialize s3
  s3_initialize_aws();
  s3_client.reset(s3_create_client_ptr());

  last = left_id;  // last is exclusive

  for (uint64_t i = 0; i < read_ahead; ++i) {
    pref_sem.signal();
  }

  sem_init(&semaphore, 0, 0);

  thread = new std::thread(std::bind(&S3SparseIterator::thread_function, this));
}

int to_delete = -1;

const void* S3SparseIterator::get_next_fast() {
  // we need to delete entry
  if (to_delete != -1) {
    std::cout << "get_next_fast::Deleting entry: " << to_delete
      << std::endl;
    list_strings.erase(to_delete);
    std::cout << "get_next_fast::Deleted entry: " << to_delete
      << std::endl;
  }
  
  sem_wait(&semaphore);
  ring_lock.lock();

  auto ret = minibatches_list.pop();
  
  uint64_t ring_size = minibatches_list.size();
  ring_lock.unlock();

  if (ret.second != -1) {
    std::cout << "get_next_fast::ret.second: " << ret.second << std::endl;
  }

  to_delete = ret.second;

  if (ring_size < 20000 && pref_sem.getvalue() < (int)read_ahead) {
    std::cout << "get_next_fast::pref_sem.signal!!!" << std::endl;
    pref_sem.signal();
  }

  return ret.first;
}

void S3SparseIterator::push_samples(std::ostringstream* oss) {
  uint64_t n_minibatches = s3_rows / minibatch_rows;

  std::cout << "n_minibatches: " << n_minibatches << std::endl;

  // save s3 object into list of string
  std::chrono::steady_clock::time_point start =
    std::chrono::steady_clock::now();
  list_strings[str_version] = oss->str();
  delete oss;
  std::chrono::steady_clock::time_point finish =
    std::chrono::steady_clock::now();
  uint64_t elapsed_ns =
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        finish-start).count();
  std::cout << "oss->str() time (ns): " << elapsed_ns << std::endl;

  auto str_iter = list_strings.find(str_version);

  ring_lock.lock();
  // create a pointer to each minibatch within s3 object and push it

  const void* s3_data = reinterpret_cast<const void*>(str_iter->second.c_str());
  int s3_obj_size = load_value<int>(s3_data);
  assert(s3_obj_size > 0 && s3_obj_size < 100 * 1024 * 1024);
  int num_samples = load_value<int>(s3_data);
  assert(num_samples > 0 && num_samples < 1000000);
  std::cout << "push_samples s3_obj_size: " << s3_obj_size << " num_samples: " << num_samples << std::endl;
  for (uint64_t i = 0; i < n_minibatches; ++i) {
    (void)load_value<FEATURE_TYPE>(s3_data); // read label
    int num_values = load_value<int>(s3_data); 
    assert(num_values > 0 && num_values < 1000000);
    
    // if it's the last minibatch in object we mark it so it can be deleted
    int is_last = ((i + 1) == n_minibatches) ? str_version : -1;

    minibatches_list.add(std::make_pair(s3_data, is_last));
    sem_post(&semaphore);
    
    // advance until the next minibatch
    advance_ptr(s3_data, num_values * (sizeof(int) + sizeof(FEATURE_TYPE))); // every sample has index and value
  }
  ring_lock.unlock();
  str_version++;
}

void S3SparseIterator::thread_function() {
  std::cout << "Building S3 deser. with size: "
    << std::endl;

  uint64_t count = 0;
  while (1) {
    // if we can go it means there is a slot
    // in the ring
    std::cout << "Waiting for pref_sem" << std::endl;
    pref_sem.wait();
    std::cout << "Getting object. count: " << count++ << std::endl;

    std::ostringstream* s3_obj;
try_start:
    try {
      std::cout << "S3SparseIterator: getting object" << std::endl;
      std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
      s3_obj = s3_get_object_fast(last, *s3_client, S3_SPARSE_BUCKET);
      std::chrono::steady_clock::time_point finish =
        std::chrono::steady_clock::now();
      uint64_t elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            finish-start).count();

      std::cout << "Get s3 obj took (us): " << (elapsed_ns / 1000.0) << std::endl;
      double MBps = (1.0 * (32812.5*1024.0) / elapsed_ns) / 1024 / 1024 * 1000 * 1000 * 1000;
      std::cout << "Get s3 obj took (us): " << (elapsed_ns / 1000.0)
        << " size (KB): " << 32812.5
        << " bandwidth (MB/s): " << MBps
        << std::endl;
    } catch(...) {
      std::cout << "S3SparseIterator: error in s3_get_object" << std::endl;
      goto try_start;
      exit(-1);
    }
    
    // update index
    last++;
    if (last == right_id)
      last = left_id;

    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
    push_samples(s3_obj);
    std::chrono::steady_clock::time_point finish2 =
      std::chrono::steady_clock::now();
    uint64_t elapsed_ns2 =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          finish2-start).count();
    std::cout << "pushing took (ns): " << elapsed_ns2 << " at (us) " << get_time_us() << std::endl;
  }
}

