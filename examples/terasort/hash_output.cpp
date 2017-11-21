#include "hash_output.hpp"
#include <functional>
#include <algorithm>
#include <sstream>

namespace cirrus_terasort {
	thread_hash_file_output::thread_hash_file_output(std::string fn) : _file_name(fn) {
		_output_stream.open(_file_name, std::ios_base::app);
	}

	thread_hash_file_output::~thread_hash_file_output() {
		_output_stream.flush();
		_output_stream.close();
	}

	const std::string thread_hash_file_output::file_name() const {
		return _file_name;
	}

	void thread_hash_file_output::write(std::shared_ptr<record>& rec) {
		_output_stream << rec->raw_data() << "\n";
	}

	thread_hash_output::thread_hash_output(std::thread::id i, uint32_t hb, uint32_t hm, std::string pd) :
		_id(i), _hash_bytes(hb), _hash_modulus(hm), _parent_dir(pd) {
		std::stringstream ss;
		ss << _id;
		_cached_id = ss.str();
	}

	thread_hash_output::~thread_hash_output() {
		for(auto& p : _prefix_2_file_map) p.second.reset();
	}

	const uint32_t thread_hash_output::hash_bytes() const {
		return _hash_bytes;
	}

	const uint32_t thread_hash_output::hash_modulus() const {
		return _hash_modulus;
	}

	void thread_hash_output::write(std::shared_ptr<record>& rec) {
		uint32_t hashed_value = std::hash<std::string>{}(rec->raw_data().substr(0, _hash_bytes)) % _hash_modulus;
		if(!_prefix_2_file_map.count(hashed_value))
			_prefix_2_file_map.emplace(hashed_value,
				std::make_shared<thread_hash_file_output>(_parent_dir + "/" + 
							_cached_id + "_" + 
							std::to_string(hashed_value) + ".txt"));
		_prefix_2_file_map.at(hashed_value)->write(rec);
	}

	hash_output::hash_output(std::shared_ptr<config> c) : _conf(c) {
		
	}
	
	hash_output::~hash_output() {
		for(auto& p: _output_map) p.second.reset();
	}

	void hash_output::write(std::thread::id curr_id, std::shared_ptr<record>& rec) {
		uint32_t hashed_id = std::hash<std::thread::id>{}(curr_id);
		_map_mutex.lock();
		if(!_output_map.count(hashed_id))
			_output_map.emplace(hashed_id,
				std::make_shared<thread_hash_output>(curr_id, _conf->hash_bytes(), _conf->hash_modulus(),
					_conf->hash_output_dir()));
		_map_mutex.unlock();
		_output_map.at(hashed_id)->write(rec);
	}
}
