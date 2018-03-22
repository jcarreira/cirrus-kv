#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <iostream>
#include <fstream>
#include <thread>

#include "../S3.hpp"
#include "../config.hpp"

static constexpr char test_prefix[] = "TEST_";
static constexpr INT_TYPE num_loops = 100;

int main(int argc, char* argv[]) {
  s3_initialize_aws();

  Aws::S3::S3Client* s3_client = s3_create_client_ptr();

  std::string dummy;
  for(INT_TYPE obj_size = 1; obj_size <= 10; obj_size++) {
    long double total_time = 0, bytes = 0;
    for(INT_TYPE i = 0; i < num_loops; i++) {
      bytes += obj_size * 1'000'000;
      auto start = std::chrono::high_resolution_clock::now();
      dummy += s3_get_object(test_prefix + std::to_string(obj_size) + "_MB",
        *s3_client, s3_bucket_name)[0];
      auto end = std::chrono::high_resolution_clock::now();
      long double time = std::chrono::duration_cast
        <std::chrono::microseconds>(end - start).count();
      total_time += time;
    }
    std::cout << "Get rate for " << obj_size << " MB objects: " <<
      (bytes / 1e6) / (total_time / 1e6) << " MB/s" << '\n';
  }

  /*for(INT_TYPE obj_size = 1; obj_size < 10; obj_size++) {
    long double total_time = 0, bytes = 0;
    for(INT_TYPE i = 0; i < num_loops; i++) {
      bytes += obj_size * 100 * 1'000;
      auto start = std::chrono::high_resolution_clock::now();
      dummy += s3_get_object(test_prefix + std::to_string(obj_size * 100) + "_KB",
        *s3_client, s3_bucket_name)[0];
      auto end = std::chrono::high_resolution_clock::now();
      long double time = std::chrono::duration_cast
        <std::chrono::microseconds>(end - start).count();
      total_time += time;
    }
    std::cout << "Get rate for " << obj_size * 100 << " KB objects: " <<
      (bytes / 1e6) / (total_time / 1e6) << " MB/s" << '\n';
  }*/
  std::cout << dummy << '\n';

  s3_shutdown_aws();
}
