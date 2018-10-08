#ifndef EXAMPLES_TERASORT_MP_SERIALIZATION_HPP_
#define EXAMPLES_TERASORT_MP_SERIALIZATION_HPP_

#include <string>

#include "object_store/FullBladeObjectStore.h"

namespace cirrus_terasort {

std::string string_deserializer(const void* data, const uint64_t size);

class string_serializer : public cirrus::Serializer<std::string> {
 public:
        uint64_t size(const std::string& s) const override;
        void serialize(const std::string& s, void* mem) const override;
};

}  // namespace cirrus_terasort

#endif  // EXAMPLES_TERASORT_MP_SERIALIZATION_HPP_
