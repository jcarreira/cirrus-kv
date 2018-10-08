#ifndef CIRRUS_TERASORT_SORT_LAMBDA_HPP
#define CIRRUS_TERASORT_SORT_LAMBDA_HPP

#include <vector>
#include <memory>
#include <future>
#include "record.hpp"
#include "sort_input.hpp"

namespace cirrus_terasort {
	void sort_lambda(std::shared_ptr<sort_input> input, std::promise<std::vector<std::shared_ptr<record>>> p);
	void merge_lambda(std::vector<std::shared_ptr<record>>& vec1, std::vector<std::shared_ptr<record>>& vec2, std::promise<std::vector<std::shared_ptr<record>>> p);
}

#endif
