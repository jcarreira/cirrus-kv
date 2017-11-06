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
		std::string _file_name;
	public:
		thread_hash_file_output(std::string fn);
		
		const std::string file_name() const;
		void write(record rec);
	};
	
	class thread_hash_output {
	private:
		std::string _parent_dir;
		uint32_t _hash_bytes;
		std::thread::id _id;
		std::string _cached_id;
		std::map<uint32_t, thread_hash_file_output> _prefix_2_file_map;
	public:
		thread_hash_output(std::thread::id i, uint32_t hb, std::string pd);
		
		const uint32_t hash_bytes() const;
		const std::string parent_dir() const;
		void write(record rec);
	};

	class hash_output {
		// maps a std::thread::id to a maps of prefixes to file names
		typedef std::map<uint32_t, thread_hash_output> hash_output_map_type;
		std::shared_ptr<config> _conf;
		hash_output_map_type _output_map;
		std::mutex _map_mutex;
	public:
		hash_output(std::shared_ptr<config> c);
	
		void write(std::thread::id curr_id, record rec);
	};
}

#endif
