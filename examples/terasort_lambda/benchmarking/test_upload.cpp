#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <iostream>
#include <fstream>
#include <thread>

#include "../S3.hpp"
#include "../config.hpp"

static constexpr char test_prefix[] = "TEST_";

std::string random_string(INT_TYPE len) {
  auto rand_char = []() {
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "01234567890";
    return chars[rand() % (sizeof(chars) - 1)];
  };
  std::string ret;
  while(len--) ret += rand_char();
  return ret;
}

int main(int argc, char* argv[]) {
	s3_initialize_aws();

	Aws::S3::S3Client* s3_client = s3_create_client_ptr();

        for(INT_TYPE obj_size = 1; obj_size <= 10; obj_size++) {
          std::cout << "Uploading a " << obj_size << " MB object" << '\n';
          std::string obj = random_string(obj_size * 1'000'000);
          s3_put_object(test_prefix + std::to_string(obj_size) + "_MB",
              *s3_client, s3_bucket_name, obj);
        }

        for(INT_TYPE obj_size = 1; obj_size < 10; obj_size++) {
          std::cout << "Uploading a " << obj_size * 100 << " KB object" << '\n';
          std::string obj = random_string(obj_size * 100 * 1'000);
          s3_put_object(test_prefix + std::to_string(obj_size * 100) + "_KB",
              *s3_client, s3_bucket_name, obj);
        }

	s3_shutdown_aws();
}
