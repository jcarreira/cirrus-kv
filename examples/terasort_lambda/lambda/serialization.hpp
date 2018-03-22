#ifndef SERIALIZATION_HPP
#define SERIALIZATION_CPP

#include <string>

#include "object_store/FullBladeObjectStore.h"

std::string string_deserializer(const void* data, const uint64_t size);

class string_serializer : public cirrus::Serializer<std::string> {
  public:
    uint64_t size(const std::string& s) const override;
    void serialize(const std::string& s, void* mem) const override;
};

#endif

