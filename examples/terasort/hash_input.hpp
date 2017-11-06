#ifndef CIRRUS_TERASORT_HASH_INPUT_HPP
#define CIRRUS_TERASORT_HASH_INPUT_HPP

#include "config.hpp"
#include "record.hpp"
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <mutex>

namespace cirrus_terasort {
	class hash_input {
	private:
		std::shared_ptr<config> _conf;
		std::string _input_file_name;
		std::ifstream _input_stream;
		std::mutex _read_mutex;
		
		uint32_t _read_chunk_length;
	public:
		hash_input(const std::shared_ptr<config> c, std::string fn);
		hash_input(const hash_input& in) = delete;
		hash_input& operator=(const hash_input& in) = delete;
		~hash_input();

		const std::string input_file_name() const;
		const std::vector<record> read_chunk();
		const uint32_t read_chunk_length() const;
	};
}

#endif
