#ifndef CIRRUS_TERASORT_SERIALIZATION_HPP
#define CIRRUS_TERASORT_SERIALIZATION_HPP

#include "object_store/FullBladeObjectStore.h"
#include <string>

namespace cirrus_terasort {

	std::string string_deserializer(const void* data, const uint64_t size);

	class string_serializer : public cirrus::Serializer<std::string> {
	public:
		uint64_t size(const std::string& s) const override;
		void serialize(const std::string& s, void* mem) const override;
	};
}

#endif
