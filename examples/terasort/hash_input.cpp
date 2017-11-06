#include "hash_input.hpp"
#include <stdexcept>

namespace cirrus_terasort {
	hash_input::hash_input(const std::shared_ptr<config> c, std::string fn) {
		_input_file_name = fn;
		_input_stream.open(_input_file_name, std::ios::binary);
		if(_input_stream.fail())
			throw std::runtime_error("cannot read input file or input file does not exist.");
		_conf = c;
		_read_chunk_length = (record_size + 1) * _conf->num_input_chunks();
	}

	hash_input::~hash_input() {
		_input_stream.close();
	}

	const std::string hash_input::input_file_name() const {
		return _input_file_name;
	}

	const uint32_t hash_input::read_chunk_length() const {
		return _read_chunk_length;
	}

	const std::vector<record> hash_input::read_chunk() {
		std::vector<char> buf(_read_chunk_length, 0);
		std::vector<record> ret{};
		_read_mutex.lock();
		_input_stream.read(buf.data(), buf.size());
		uint32_t gcount = _input_stream.gcount();
		_read_mutex.unlock();
		if(!gcount) return ret;
		for(uint32_t i = 0; i < _conf->num_input_chunks() && (i + 1) * (record_size + 1) <= gcount; i++)
			ret.push_back(record(std::string(buf.begin() + i * (record_size + 1),
							buf.begin() + i * (record_size + 1) + record_size)));
		return ret;
	}
}
