#ifndef CIRRUS_TERASORT_SORT_LAMBDA_HPP
#define CIRRUS_TERASORT_SORT_LAMBDA_HPP

#include "config_instance.hpp"

#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include <atomic>
#include <future>

namespace cirrus_terasort {
	
	class sort_lambda {
	private:
		std::vector<std::shared_ptr<std::ifstream>> _file_list;
		std::atomic<INT_TYPE> _thread_access_counter;
	public:
		sort_lambda(std::vector<std::string>& ifl);
		~sort_lambda();

		std::vector<std::shared_ptr<std::string>> get_records();
	};

	void sorter(std::shared_ptr<sort_lambda> sl, std::promise<std::vector<std::shared_ptr<std::string>>> p);
	void merger(std::vector<std::shared_ptr<std::string>>& vec1, std::vector<std::shared_ptr<std::string>>& vec2,
		std::promise<std::vector<std::shared_ptr<std::string>>> p);
}

#endif
