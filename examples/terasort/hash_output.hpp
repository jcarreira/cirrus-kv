#ifndef CIRRUS_TERASORT_OUTPUT_HPP
#define CIRRUS_TERASORT_OUTPUT_HPP

#include "hash_input.hpp"
#include "record.hpp"
#include <memory>
#include <map>
#include <set>
#include <thread>
#include <mutex>
#include <iostream>

namespace cirrus_terasort {
	class thread_hash_file_output {
	private:
		std::ofstream _output_stream;
		std::string _file_name;
	public:
		thread_hash_file_output(std::string fn);
		~thread_hash_file_output();
		
		const std::string file_name() const;
		void write(std::shared_ptr<record>& rec);
	};
	
	class thread_hash_output {
	private:
		std::string _parent_dir;
		uint32_t _hash_bytes;
		uint32_t _hash_modulus;
		std::thread::id _id;
		std::string _cached_id;
		std::map<uint32_t, std::shared_ptr<thread_hash_file_output>> _prefix_2_file_map;
	public:
		thread_hash_output(std::thread::id i, uint32_t hb, uint32_t hm, std::string pd);
		~thread_hash_output();
		
		const uint32_t hash_bytes() const;
		const uint32_t hash_modulus() const;
		const std::string parent_dir() const;
		void write(std::shared_ptr<record>& rec);
	};

	class hash_output {
		// maps a std::thread::id to a maps of prefixes to file names
		typedef std::map<uint32_t, std::shared_ptr<thread_hash_output>> hash_output_map_type;
		std::shared_ptr<config> _conf;
		hash_output_map_type _output_map;
		std::mutex _map_mutex;
	public:
		hash_output(std::shared_ptr<config> c);
		~hash_output();
	
		void write(std::thread::id curr_id, std::shared_ptr<record>& rec);
	};
}

#endif
