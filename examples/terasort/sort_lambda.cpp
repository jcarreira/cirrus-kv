#include "sort_lambda.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>

namespace cirrus_terasort {
	void sort_lambda(std::shared_ptr<sort_input> input, std::promise<std::vector<std::shared_ptr<record>>> p) {
		std::vector<std::shared_ptr<record>> recs = input->read_files(std::this_thread::get_id());
		uint32_t recs_size = recs.size();
		auto sort_start = std::chrono::high_resolution_clock::now();
		std::sort(recs.begin(), recs.end(), [] (const std::shared_ptr<record>& lhs, const std::shared_ptr<record>& rhs) {
			return lhs->raw_data() < rhs->raw_data();
		});
		p.set_value(std::move(recs));
		auto sort_end = std::chrono::high_resolution_clock::now();
		std::cout << "Sorting a vector of size " << recs_size << " took " << std::chrono::duration_cast<std::chrono::seconds>(sort_end - sort_start).count() << " seconds" << std::endl;
	}

	void merge_lambda(std::vector<std::shared_ptr<record>>& vec1, std::vector<std::shared_ptr<record>>& vec2, std::promise<std::vector<std::shared_ptr<record>>> p) {
		auto merge_start = std::chrono::high_resolution_clock::now();
		std::vector<std::shared_ptr<record>> ret;
		std::merge(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(), std::back_inserter(ret), [](const std::shared_ptr<record>& lhs, const std::shared_ptr<record>& rhs) {
			return lhs->raw_data() < rhs->raw_data();
		});
		p.set_value(std::move(ret));
		auto merge_end = std::chrono::high_resolution_clock::now();
		std::cout << "Merging two vectors of size " << vec1.size() << " and " << vec2.size() << " took "
			<< std::chrono::duration_cast<std::chrono::seconds>(merge_end - merge_start).count() << " seconds" << std::endl;
	}
}
