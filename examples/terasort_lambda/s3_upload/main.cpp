#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <iostream>
#include <fstream>
#include <thread>

#include "../S3.hpp"
#include "disk_to_s3.hpp"

using INT_TYPE = uint64_t;

int main(int argc, char* argv[]) {
	s3_initialize_aws();

	Aws::S3::S3Client* s3_client = s3_create_client_ptr();
	
	read_info* ri = new read_info(data_file_name);
	std::vector<std::thread> all;
	for (INT_TYPE i = 0; i < read_threads; i++)
		all.push_back(std::thread(reader, ri, s3_client));

	for (INT_TYPE i = 0; i < read_threads; i++)
		all[i].join();

	s3_shutdown_aws();
}
