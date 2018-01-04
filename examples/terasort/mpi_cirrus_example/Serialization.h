#ifndef _SERIALIZATION_H_
#define _SERIALIZATION_H_

class serializer_string : public cirrus::Serializer<std::string> {
 public:
    serializer_string();
    uint64_t size(const std::string& object) const override;

    void serialize(const std::string& object, void *mem) const override;
};

serializer_string::serializer_string() {
}

uint64_t serializer_string::size(const std::string& s) const {
    return sizeof(char) * s.size();
}

void serializer_string::serialize(const std::string& s, void* mem) const {
    std::memcpy(mem, s.c_str(), size(s));
}

#endif // _SERIALIZATION_H_
