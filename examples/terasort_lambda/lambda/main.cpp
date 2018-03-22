#include <aws/core/Aws.h>
#include <memory>
#include <cstdlib>
#include <utility>
#include <regex>

#include "../S3.hpp"
#include "hash.hpp"
#include "sort.hpp"
#include "serialization.hpp"

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"

std::pair<bool, std::string> object_exists(std::shared_ptr<Aws::S3::S3Client> s3_client,
    const std::string& s, bool put, const std::string& data) {
  try {
    std::string read_data = s3_get_object(s, *s3_client, s3_bucket_name);
    return std::make_pair(true, read_data);
  }
  catch(...) {
    try {
      if (put)
        s3_put_object(s, *s3_client, s3_bucket_name, data);
      return std::make_pair(false, "");
    }
    catch(...) {
      return std::make_pair(false, "");
    }
  }
}

int main(int argc, char* argv[]) {
  s3_initialize_aws();

  const char* proc_num_str = argv[proc_num_arg];
  int proc_num = atoi(proc_num_str);

  cirrus::TCPClient tcp_client;
  string_serializer str_ser;
  std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
    store =
    std::make_shared<
    cirrus::ostore::FullBladeObjectStoreTempl<std::string>>(
        cirrus_ip, cirrus_port, &tcp_client, str_ser,
        string_deserializer);

  std::shared_ptr<Aws::S3::S3Client> s3_client(s3_create_client_ptr());

  if(proc_num < rh_nodes) {

    INT_TYPE total_keys = num_records / s3_read_chunk_size,
             keys_per_process = total_keys / rh_nodes,
             process_index = proc_num;
    INT_TYPE start = process_index * keys_per_process,
             end = (process_index + 1) * keys_per_process;
    std::vector<INT_TYPE> initial_counter_list;

    std::pair<bool, std::string> active_check = object_exists(s3_client,
        hash_active_flag_prefix + std::to_string(proc_num), true, "");
    std::pair<bool, std::string> done_check = object_exists(s3_client,
        hash_done_flag_prefix + std::to_string(proc_num), false, "");
    if(active_check.first || done_check.first)
      return 0;
    std::pair<bool, std::string> resume_check = object_exists(s3_client,
        hash_progress_flag_prefix + std::to_string(proc_num), false, "");
    if(resume_check.first) {
      const std::string& data = resume_check.second;
      std::regex re("\\s+");
      std::vector<std::string> result { std::sregex_token_iterator(data.begin(),
          data.end(), re, -1), {} };
      start = std::stoul(result[result.size() - 1]);
      INT_TYPE pos = 0;
      for (; pos < result.size() - 1; pos++)
        initial_counter_list.push_back(std::stoul(result[pos]));
      for (; pos < sort_nodes; pos++)
        initial_counter_list.push_back(0);
    }
    else {
      for (INT_TYPE i = 0; i < sort_nodes; i++)
        initial_counter_list.push_back(0);
    }

    std::cout << "Start: " << start << std::endl;
    std::cout << "Counter list: ";
    for (const INT_TYPE& i : initial_counter_list)
      std::cout << i << ' ';
    std::cout << std::endl;
    std::shared_ptr<hash_lambda> hl =
      std::make_shared<hash_lambda>
      (store, process_index, start, end, initial_counter_list);
    std::vector<std::thread> all;
    for (INT_TYPE i = 0; i < rh_threads; i++)
      all.push_back(std::thread(hasher, hl, s3_client, store));

    for (INT_TYPE i = 0; i < rh_threads; i++)
      all[i].join();
    
    hl->finish(store);

    hl->print_avg_stats();
    
    if(hl->current() >= hl->end()) {
      object_exists(s3_client, hash_done_flag_prefix + std::to_string(proc_num), true, "");
      s3_delete_object(hash_progress_flag_prefix + std::to_string(proc_num), *s3_client, s3_bucket_name);
    }
    else {
      std::string to_put;
      for (const INT_TYPE& i : hl->counter_list())
        to_put += std::to_string(i) + " ";
      to_put += std::to_string(hl->current());
      s3_delete_object(hash_progress_flag_prefix + std::to_string(proc_num), *s3_client, s3_bucket_name);
      object_exists(s3_client, hash_progress_flag_prefix + std::to_string(proc_num),
          true, to_put);
    }
    s3_delete_object(hash_active_flag_prefix + std::to_string(proc_num), *s3_client, s3_bucket_name);
  }
  else if (rh_nodes <= proc_num && proc_num < rh_nodes + sort_nodes) {
    proc_num -= rh_nodes;

    std::vector<INT_TYPE> offsets;
    INT_TYPE next_sort_index_offset, next_index_offset;
    std::pair<bool, std::string> active_check = object_exists(s3_client,
        sort_active_flag_prefix + std::to_string(proc_num), true, "");
    std::pair<bool, std::string> done_check = object_exists(s3_client,
        sort_done_flag_prefix + std::to_string(proc_num), false, "");
    if(active_check.first || done_check.first)
      return 0;
    std::pair<bool, std::string> resume_check = object_exists(s3_client,
        sort_progress_flag_prefix + std::to_string(proc_num), false, "");
    if (resume_check.first) {
      const std::string& data = resume_check.second;
      std::regex re("\\s+");
      std::vector<std::string> result { std::sregex_token_iterator(data.begin(),
          data.end(), re, -1), {} };
      next_index_offset = std::stoul(result[result.size() - 1]);
      next_sort_index_offset = std::stoul(result[result.size() - 2]);
      INT_TYPE pos = 0;
      for (; pos < result.size() - 2; pos++)
        offsets.push_back(std::stoul(result[pos]));
      for (; pos < rh_nodes; pos++)
        offsets.push_back(0);
    }
    else {
      for (INT_TYPE _ = 0; _ < rh_nodes; _++)
        offsets.push_back(0);
      next_index_offset = next_sort_index_offset = 0;
    }

    std::cout << "Next index offset: " << next_index_offset << std::endl;
    std::cout << "Sort next index offset: " << next_sort_index_offset << std::endl;
    std::cout << "Index offset list: ";
    for (const INT_TYPE& i : offsets)
      std::cout << i << ' ';
    std::cout << std::endl;
    std::shared_ptr<sort_lambda> sl = std::make_shared<sort_lambda>(store, s3_client, proc_num,
        offsets, next_index_offset, next_sort_index_offset);
    std::vector<std::thread> all;
    for (INT_TYPE _ = 0; _ < sort_threads; _++)
      all.push_back(std::thread(&sorter, sl, s3_client));

    for (INT_TYPE _ = 0; _ < all.size(); _++)
      all[_].join();

    sl->finish();

    sl->print_avg_stats();

    if(sl->next_index() >= rh_nodes) {
      object_exists(s3_client, sort_done_flag_prefix + std::to_string(proc_num), true, "");
      s3_delete_object(sort_progress_flag_prefix + std::to_string(proc_num), *s3_client, s3_bucket_name);
    }
    else {
      std::string to_put;
      for (INT_TYPE _ = 0; _ < sl->index_list().size(); _++)
        to_put += std::to_string(sl->index_list()[_] - sl->initial_index_list()[_]) + " ";
      to_put += std::to_string(sl->current_sort_index() - sl->initial_sort_index()) + " "
        + std::to_string(sl->next_index());
      s3_delete_object(sort_progress_flag_prefix + std::to_string(proc_num), *s3_client, s3_bucket_name);
      object_exists(s3_client, sort_progress_flag_prefix + std::to_string(proc_num),
          true, to_put);
    }
    s3_delete_object(sort_active_flag_prefix + std::to_string(proc_num), *s3_client, s3_bucket_name);
  }

  s3_shutdown_aws();
}
