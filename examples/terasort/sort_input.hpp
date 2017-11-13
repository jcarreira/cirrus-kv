#ifndef CIRRUS_TERASORT_SORT_INPUT_HPP
#define CIRRUS_TERASORT_SORT_INPUT_HPP

#include <memory>
#include <experimental/filesystem>
#include <map>
#include <vector>
#include <fstream>
#include <thread>
#include <atomic>
#include "config.hpp"
#include "record.hpp"

namespace cirrus_terasort {
	
	class sort_input {
	private:
		std::shared_ptr<config> _u_conf;
		std::atomic<uint32_t> _index_counter;
		std::vector<std::vector<std::shared_ptr<std::ifstream>>> _file_split;
	public:
		sort_input(std::shared_ptr<config> uc);
		sort_input(const sort_input& si) = delete;
		sort_input& operator=(const sort_input& si) = delete;

		std::vector<record> read_files(std::thread::id tid);
	};
}

#endif
