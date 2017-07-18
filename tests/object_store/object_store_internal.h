#ifndef TESTS_OBJECT_STORE_OBJECT_STORE_INTERNAL_H_
#define TESTS_OBJECT_STORE_OBJECT_STORE_INTERNAL_H_

#include <utility>
#include <memory>

#include "common/Serializer.h"

namespace cirrus {

/**
  * Struct used in many of the tests.
  */
template<unsigned int SIZE>
struct Dummy {
    char data[SIZE];
    int id;
    explicit Dummy(int id = 1492) : id(id) {}
};

template<class T>
class serializer_simple : public cirrus::Serializer<T> {
 public:
    serializer_simple();
    uint64_t size(const T& object) override;

    void serialize(const T& object, void *mem) override;
};

template<typename T>
serializer_simple<T>::serializer_simple() {
}

template<typename T>
uint64_t serializer_simple<T>::size(const T& object) {
    return sizeof(object);
}

template<typename T>
void serializer_simple<T>::serialize(const T& object, void *mem) {
    std::memcpy(mem, &object, sizeof(object));
}

/* Takes a pointer to raw mem passed in and returns as object. */
template<typename T, unsigned int SIZE>
T deserializer_simple(void* data, unsigned int /* size */) {
    T *ptr = reinterpret_cast<T*>(data);
    T ret;
    std::memcpy(&ret, ptr, SIZE);
    return ret;
}

}  // namespace cirrus

#endif  // TESTS_OBJECT_STORE_OBJECT_STORE_INTERNAL_H_
