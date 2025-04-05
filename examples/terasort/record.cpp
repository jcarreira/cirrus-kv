#include "record.hpp"
#include <stdexcept>

namespace cirrus_terasort {
	record::record(const std::string&& rd) {
		if(rd.length() != record_size)
			throw std::runtime_error("record of bad size.");
		_raw_data = rd;
	}

	const std::string& record::raw_data() const {
		return _raw_data;
	}
}
