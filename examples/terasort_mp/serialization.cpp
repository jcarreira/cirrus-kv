#include "serialization.hpp"
#include <cstring>

namespace cirrus_terasort {
	
	std::string string_deserializer(const void* data, const uint64_t size) {
		std::string ret;
		ret.assign(reinterpret_cast<const char*>(data), size);
		return ret;
	}

	uint64_t string_serializer::size(const std::string& s) const {
		return sizeof(char) * s.size();
	}

	void string_serializer::serialize(const std::string& s, void* mem) const {
		std::memcpy(mem, s.c_str(), size(s));
	}
}
