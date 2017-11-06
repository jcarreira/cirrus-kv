#include "hash_lambda.hpp"
#include <vector>
#include <string>

namespace cirrus_terasort {
	void hash_lambda(std::shared_ptr<hash_input> input, std::shared_ptr<hash_output> output) {
		std::vector<cirrus_terasort::record> recs;
		while((recs = input->read_chunk()).size()) {
			for(cirrus_terasort::record& rec : recs)
				output->write(std::this_thread::get_id(), rec);
		}
	}
}
