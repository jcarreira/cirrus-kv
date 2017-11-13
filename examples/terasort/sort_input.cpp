#include "sort_input.hpp"
#include <algorithm>
#include <streambuf>
#include <stdexcept>
#include <iostream>

namespace cirrus_terasort {
	sort_input::sort_input(std::shared_ptr<config> uc) : _u_conf(uc), _file_split(std::vector<std::vector<std::shared_ptr<std::ifstream>>>{}),
		 _index_counter(0) {
		namespace fs = std::experimental::filesystem;
		fs::directory_iterator iter = fs::directory_iterator(_u_conf->hash_output_dir());
		uint32_t counter = 0, num_files = std::distance(iter, fs::directory_iterator{}),
			per_thread = std::max(num_files / _u_conf->sort_nodes(), 1u);
		iter = fs::directory_iterator(_u_conf->hash_output_dir());
		for(auto& p: iter) {
			std::shared_ptr<std::ifstream> i = std::make_shared<std::ifstream>(p.path().string().c_str(), std::ios::binary);
			if(counter++ % per_thread == 0) _file_split.push_back(std::vector<std::shared_ptr<std::ifstream>>{});
			_file_split.back().push_back(i);
		}
	}

	std::vector<record> sort_input::read_files(std::thread::id tid) {
		uint32_t hashed_tid = std::hash<std::thread::id>{}(tid);
		std::vector<record> ret;
		std::vector<std::shared_ptr<std::ifstream>>& vec = _file_split[_index_counter++];
		for(std::shared_ptr<std::ifstream> i: vec) {
			std::vector<char> buf(101, 0);
			while(i->read(buf.data(), buf.size()))
				ret.push_back(std::string(buf.begin(), buf.end() - 1));
			if(i->gcount() != 0) throw std::runtime_error("bad file read size.");
		}
		return ret;
	}
}
