#ifndef SRC_COMMON_SERIALIZER_H_
#define SRC_COMMON_SERIALIZER_H_


namespace cirrus {

template<class T>
class Serializer {
 public:
    Serializer() {}
    virtual uint64_t size(const T& object) = 0;

    virtual void serialize(const T& object, void* mem) = 0;
};

}  // namespace cirrus

#endif  // SRC_COMMON_SERIALIZER_H_
