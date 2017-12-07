#ifndef CIRRUS_SERIALIZATION_HASH_LAMBDA_HPP
#define CIRRUS_SERIALIZATION_HASH_LAMBDA_HPP

#include "config_instance.hpp"

#include "object_store/FullBladeObjectStore.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <fstream>
#include <utility>
#include <map>
#include <vector>

namespace cirrus_terasort {

	class hash_lambda {
	private:
		std::atomic<INT_TYPE> _counter;
		std::map<std::pair<std::thread::id, INT_TYPE>,
			std::shared_ptr<std::ofstream>> _file_map;
		std::mutex _map_mutex;
		INT_TYPE _start, _end;
	public:
		hash_lambda(INT_TYPE s, INT_TYPE e);
		~hash_lambda();
		
		void add_to_map(std::vector<std::tuple<std::thread::id, INT_TYPE, std::shared_ptr<std::ofstream>>>& vec);
		std::shared_ptr<std::ofstream> get_stream(std::pair<std::thread::id, INT_TYPE> p);
		INT_TYPE next_counter();
		INT_TYPE start();
		INT_TYPE end();
	};

	void hasher(std::shared_ptr<hash_lambda> hl, INT_TYPE p,
		std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>> store);
}

#endif
