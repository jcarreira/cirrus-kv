#ifndef CIRRUS_TERASORT_HASH_LAMBDA_HPP
#define CIRRUS_TERASORT_HASH_LAMBDA_HPP

#include "record.hpp"
#include "hash_input.hpp"
#include "hash_output.hpp"
#include <memory>

namespace cirrus_terasort {
	void hash_lambda(std::shared_ptr<hash_input> input, std::shared_ptr<hash_output> output);
}

#endif
