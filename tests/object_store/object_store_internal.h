#ifndef _OBJECT_STORE_INTERNAL_H_
#define _OBJECT_STORE_INTERNAL_H_

#include <utility>
#include <memory>

namespace cirrus {

/**
  * Struct used in many of the tests.
  */
template<unsigned int SIZE>
struct Dummy {
    char data[SIZE];
    int id;
    explicit Dummy(int id = 0) : id(id) {}
};

/* This function simply copies an object into a new portion of memory. */
template<typename T>
std::pair<std::unique_ptr<char[]>, unsigned int>
                         serializer_simple(const T& v) {
    std::unique_ptr<char[]> ptr(new char[sizeof(T)]);
    std::memcpy(ptr.get(), &v, sizeof(T));
    return std::make_pair(std::move(ptr), sizeof(T));
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

#endif  // _OBJECT_STORE_INTERNAL_H_
