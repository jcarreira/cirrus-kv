#ifndef EXAMPLES_TERASORT_MP_READ_LAMBDA_HPP_
#define EXAMPLES_TERASORT_MP_READ_LAMBDA_HPP_

#include <thread>
#include <string>
#include <mutex>
#include <fstream>
#include <atomic>
#include <memory>

#include "config_instance.hpp"
#include "serialization.hpp"

#include "object_store/FullBladeObjectStore.h"

namespace cirrus_terasort {

class read_lambda {
 private:
        std::mutex _read_mutex;
        std::string _input_file_name;
        std::ifstream _file_stream;
        std::atomic<INT_TYPE> _read_key;
        INT_TYPE _start_key;
 public:
        explicit read_lambda(INT_TYPE proc_num);
        ~read_lambda();

        INT_TYPE read_input_chunk(
                std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
                        <std::string>> ostore);
        const INT_TYPE read_key() const;
};

void reader(std::shared_ptr<read_lambda> rl,
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
                ostore);
}  // namespace cirrus_terasort

#endif  // EXAMPLES_TERASORT_MP_READ_LAMBDA_HPP_
