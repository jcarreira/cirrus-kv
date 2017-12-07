#include "sort_lambda.hpp"

#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace cirrus_terasort {

	
	sort_lambda::sort_lambda(std::vector<std::string>& ifl) {
		_thread_access_counter = 0;
		_file_list = std::vector<std::shared_ptr<std::ifstream>>{};
		for (std::string& s : ifl) {
			std::shared_ptr<std::ifstream> i =
				std::make_shared<std::ifstream>(s.c_str(), std::ios::binary);
			_file_list.push_back(i);
		}
	}

	sort_lambda::~sort_lambda() {
		for (std::shared_ptr<std::ifstream> i: _file_list) i->close();
	}

	std::vector<std::shared_ptr<std::string>> sort_lambda::get_records() {
		INT_TYPE curr_index = _thread_access_counter++,
			per_thread = _file_list.size() / config_instance::sort_threads;
		INT_TYPE start = curr_index * per_thread,
			end = curr_index != config_instance::sort_threads - 1 ?
				(curr_index + 1) * per_thread : _file_list.size();

		std::vector<std::shared_ptr<std::string>> ret;
		std::vector<std::shared_ptr<std::ifstream>> slice(_file_list.begin() + start,
								_file_list.begin() + end);

		for (std::shared_ptr<std::ifstream>& i: slice) {
			std::vector<char> buf(101, 0);
			while (i->read(buf.data(), buf.size()))
				ret.push_back(std::make_shared<std::string>(buf.begin(),
										buf.end() - 1));
			if (i->gcount() != 0)
				throw std::runtime_error("bad file read size.");
		}
	
		return ret;
	}

	void sorter(std::shared_ptr<sort_lambda> sl, std::promise<std::vector<std::shared_ptr<std::string>>> p) {
		std::vector<std::shared_ptr<std::string>> recs = sl->get_records();
		uint32_t recs_size = recs.size();
		std::sort(recs.begin(), recs.end(), [] (const std::shared_ptr<std::string>& lhs,
							const std::shared_ptr<std::string>& rhs) {
			return *lhs < *rhs;
		});
		p.set_value(std::move(recs));
	}
	void merger(std::vector<std::shared_ptr<std::string>>& vec1, std::vector<std::shared_ptr<std::string>>& vec2,
		std::promise<std::vector<std::shared_ptr<std::string>>> p) {
		std::vector<std::shared_ptr<std::string>> ret;
		std::merge(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(), std::back_inserter(ret),
			[] (const std::shared_ptr<std::string>& lhs, const std::shared_ptr<std::string>& rhs) {
			return *lhs < *rhs;
		});
		p.set_value(std::move(ret));
	}
}
