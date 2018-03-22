#include "disk_to_s3.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>

read_info::read_info(const std::string& file_name) {
        _input_file_name = file_name;
        _read_key = 0;
        _file_stream.open(_input_file_name, std::ios::binary);
}

read_info::~read_info() {
        _file_stream.close();
}

INT_TYPE read_info::read_input_chunk(Aws::S3::S3Client* client_ptr) {
	Aws::S3::S3Client& client = *client_ptr;
        INT_TYPE read_chunk_length = (record_size + 1) * s3_read_chunk_size;
        std::vector<char> buf(read_chunk_length, 0);
        std::vector<std::string> ret{};

        _read_mutex.lock();
        _file_stream.read(buf.data(), buf.size());
        INT_TYPE gcount = _file_stream.gcount();
        _read_mutex.unlock();

        if (!gcount)
                return 0;

        try {
		INT_TYPE int_key = _read_key++;
		if (int_key % update_period == 0)
			std::cout << int_key << '/' << (num_records / s3_read_chunk_size) << std::endl;
		const std::string& key = read_prefix + std::to_string(int_key);
		s3_put_object(key, client, s3_bucket_name, std::string(buf.begin(), buf.end()));
        }
        catch(...) {
                throw std::runtime_error("could not insert record.");
        }
        return gcount;
}

const INT_TYPE read_info::read_key() const {
        return _read_key;
}

void reader(read_info* ri, Aws::S3::S3Client* client_ptr) {
        INT_TYPE temp = 0;
        while ((temp = ri->read_input_chunk(client_ptr)) != 0) {}
}
