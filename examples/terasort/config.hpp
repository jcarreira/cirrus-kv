#ifndef CIRRUS_TERASORT_CONFIG_HPP
#define CIRRUS_TERASORT_CONFIG_HPP

#include <cstdint>
#include <string>

namespace cirrus_terasort {
	class config {
	public:
		// TODO: currently support for HASH_STREAMING only planned
		enum class hash_mode {
			HASH_STREAMING, // this will output _hash_nodes * _hash_buckets files
			HASH_BUCKETED // this will output _hash_buckets files
		};
	private:
		uint32_t _hash_nodes; // the number of nodes reserved for hashing
		uint32_t _hash_buckets; // the number of buckets we'll hash into (should be a power of 2)
		uint32_t _hash_bytes; // 256 * _hash_bytes = _hash_buckets
		uint32_t _hash_modulus;
		enum hash_mode _h_mode;
		
		uint32_t _num_hash_output_files; // will depend on the hash_mode

		uint32_t _num_input_chunks;

		std::string _hash_output_dir;

		uint32_t _sort_nodes;
		std::string _sort_output_dir; // inputs will be in _hash_output_dir
	public:
		config(uint32_t hn, uint32_t hbs, uint32_t hm, enum hash_mode h_m,
			uint32_t ncs, std::string hod, uint32_t sn, std::string sod);

		const uint32_t hash_nodes() const;
		const uint32_t hash_buckets() const;
		const uint32_t num_hash_output_files() const;
		const uint32_t hash_bytes() const;
		const enum hash_mode h_mode() const;
		const uint32_t num_input_chunks() const;
		const std::string hash_output_dir() const;
		const uint32_t sort_nodes() const;
		const std::string sort_output_dir() const;
		const uint32_t hash_modulus() const;
	};
}

#endif
