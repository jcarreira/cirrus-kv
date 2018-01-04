#include "read_lambda.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>

namespace cirrus_terasort {

read_lambda::read_lambda(INT_TYPE proc_num) {
        INT_TYPE per_process = config_instance::num_records /
                config_instance::num_input_files;
        _input_file_name = config_instance::input_files[proc_num];
        _read_key = _start_key = per_process * proc_num;
        _file_stream.open(_input_file_name, std::ios::binary);
}

read_lambda::~read_lambda() {
        _file_stream.close();
}

INT_TYPE read_lambda::read_input_chunk(
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
                ostore) {
        INT_TYPE read_chunk_length = (config_instance::record_size + 1) *
                config_instance::read_chunk_size;
        std::vector<char> buf(read_chunk_length, 0);
        std::vector<std::string> ret{};

        _read_mutex.lock();
        _file_stream.read(buf.data(), buf.size());
        INT_TYPE gcount = _file_stream.gcount();
        _read_mutex.unlock();

        if (!gcount)
                return 0;

        try {
                ostore->put(_read_key++, std::string(buf.begin(), buf.end()));
        }
        catch(...) {
                throw std::runtime_error("could not insert record.");
        }
        return gcount;
}

const INT_TYPE read_lambda::read_key() const {
        return _read_key;
}

void reader(std::shared_ptr<read_lambda> rl,
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
                ostore) {
        INT_TYPE temp = 0;
        while ((temp = rl->read_input_chunk(ostore)) != 0) {}
}
}  // namespace cirrus_terasort
