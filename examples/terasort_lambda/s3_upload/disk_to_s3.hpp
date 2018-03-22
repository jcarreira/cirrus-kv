#ifndef DISK_TO_S3_HPP
#define DISK_TO_S3_CPP

#include <thread>
#include <string>
#include <mutex>
#include <fstream>
#include <atomic>
#include <memory>

#include "../config.hpp"
#include "../S3.hpp"

class read_info {
 private:
        std::mutex _read_mutex;
        std::string _input_file_name;
        std::ifstream _file_stream;
        std::atomic<INT_TYPE> _read_key;
 public:
        explicit read_info(const std::string& file_name);
        ~read_info();

        INT_TYPE read_input_chunk(Aws::S3::S3Client* client_ptr);
        const INT_TYPE read_key() const;
};

void reader(read_info* ri, Aws::S3::S3Client* client_ptr);

#endif
