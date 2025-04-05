#ifndef CIRRUS_TERASORT_RECORD_HPP
#define CIRRUS_TERASORT_RECORD_HPP

#include <string>

namespace cirrus_terasort {

	const static uint32_t record_size = 100;

	class record {
	private:
		std::string _raw_data;
	public:
		record(const std::string&& rd);
		
		const std::string& raw_data() const;
	};
}

#endif
