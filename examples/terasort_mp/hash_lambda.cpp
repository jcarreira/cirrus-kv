#include "hash_lambda.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace cirrus_terasort {
	
	hash_lambda::hash_lambda(INT_TYPE s, INT_TYPE e) : _start(s), _end(e) {
		_file_map = std::map<std::pair<std::thread::id, INT_TYPE>,
			std::shared_ptr<std::ofstream>>{};
		_counter = _start;
	}

	hash_lambda::~hash_lambda() {
		for (auto& p : _file_map) p.second.reset();
	}

	void hash_lambda::add_to_map(std::vector<std::tuple<std::thread::id, INT_TYPE, std::shared_ptr<std::ofstream>>>& vec) {
		_map_mutex.lock();
		for (auto& tup : vec) _file_map[std::make_pair(std::get<0>(tup), std::get<1>(tup))] = std::get<2>(tup);
		_map_mutex.unlock();
	}

	std::shared_ptr<std::ofstream> hash_lambda::get_stream(std::pair<std::thread::id, INT_TYPE> p) {
		_map_mutex.lock();
		std::shared_ptr<std::ofstream> ret = _file_map.at(p);
		_map_mutex.unlock();
		return ret;
	}

	INT_TYPE hash_lambda::next_counter() {
		return _counter++;
	}

	INT_TYPE hash_lambda::start() {
		return _start;
	}

	INT_TYPE hash_lambda::end() {
		return _end;
	}

	void hasher(std::shared_ptr<hash_lambda> hl, INT_TYPE p,
		std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>> store) {
		std::vector<std::tuple<std::thread::id, INT_TYPE, std::shared_ptr<std::ofstream>>> data;
		for (INT_TYPE i = 0; i < config_instance::hash_modulus; i++) {
			std::stringstream ss;
			ss << std::this_thread::get_id();
			std::shared_ptr<std::ofstream> os = std::make_shared<std::ofstream>();
			os->open(std::string(config_instance::hash_output_dir) + "/" + std::to_string(p) +
				ss.str() + "_" +
				std::to_string(i) + ".txt");
			data.push_back(std::make_tuple(std::this_thread::get_id(), i, os));
		}
		hl->add_to_map(data);
		INT_TYPE curr;
		while((curr = hl->next_counter()) < hl->end()) {
			try {
				std::string chunk = store->get(curr);
				for (uint32_t i = 0; i < config_instance::read_chunk_size && (i + 1) * (config_instance::record_size + 1) <= chunk.size(); i++) {
					std::string str = chunk.substr(i * (config_instance::record_size + 1), config_instance::record_size);
					INT_TYPE mod = std::hash<std::string>{}(str.substr(0, config_instance::hash_bytes)) % config_instance::hash_modulus;
					std::shared_ptr<std::ofstream> o = hl->get_stream(std::make_pair(std::this_thread::get_id(), mod));
					*o << str << "\n";
				}
			}
			catch(...) {
				throw std::runtime_error("id not found" + std::to_string(curr) + "\n");
			}
		}
	}
}
