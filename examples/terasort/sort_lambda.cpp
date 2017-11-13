#include "sort_lambda.hpp"
#include <algorithm>
#include <iostream>

namespace cirrus_terasort {
	void sort_lambda(std::shared_ptr<sort_input> input, std::promise<std::vector<record>> p) {
		std::vector<record> recs = input->read_files(std::this_thread::get_id());
		std::sort(recs.begin(), recs.end(), [] (const record& lhs, const record& rhs) {
			return lhs.raw_data() < rhs.raw_data();
		});
		p.set_value(std::move(recs));
	}

	void merge_lambda(std::vector<record>& vec1, std::vector<record>& vec2, std::promise<std::vector<record>> p) {
		std::vector<record> ret;
		std::merge(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(), std::back_inserter(ret), [](const record& lhs, const record& rhs) {
			return lhs.raw_data() < rhs.raw_data();
		});
		p.set_value(std::move(ret));
	}
}
