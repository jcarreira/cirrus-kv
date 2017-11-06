#include "config.hpp"

namespace cirrus_terasort {
	config::config(uint32_t hn, uint32_t hbs, enum hash_mode h_m, uint32_t ncs, std::string hod, uint32_t sn, std::string sod)
		: _hash_nodes(hn), _hash_bytes(hbs), _h_mode(h_m), _num_input_chunks(ncs), _hash_output_dir(hod), _sort_nodes(sn),
			_sort_output_dir(sod) {
		_hash_buckets = _hash_bytes << 8;
		_num_hash_output_files = h_m == hash_mode::HASH_STREAMING ? _hash_nodes * _hash_buckets :
			_hash_buckets;
	}

	const uint32_t config::hash_nodes() const {
		return _hash_nodes;
	}

	const uint32_t config::hash_buckets() const {
		return _hash_buckets;
	}

	const uint32_t config::num_hash_output_files() const {
		return _num_hash_output_files;
	}

	const uint32_t config::hash_bytes() const {
		return _hash_bytes;
	}
	
	const config::hash_mode config::h_mode() const {
		return _h_mode;
	}

	const uint32_t config::num_input_chunks() const {
		return _num_input_chunks;
	}

	const std::string config::hash_output_dir() const {
		return _hash_output_dir;
	}

	const uint32_t config::sort_nodes() const {
		return _sort_nodes;
	}

	const std::string config::sort_output_dir() const {
		return _sort_output_dir;
	}
}
