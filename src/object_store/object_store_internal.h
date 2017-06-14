#ifndef _OBJECT_STORE_INTERNAL_H_
#define _OBJECT_STORE_INTERNAL_H_

#include <utility>
namespace cirrus {


template<unsigned int SIZE>
struct Dummy {
    char data[SIZE];
    int id;
    explicit Dummy(int id) : id(id) {}
};

/* This function simply copies a struct Dummy into a new portion of memory. */
template<unsigned int SIZE>
std::pair<std::unique_ptr<char[]>, unsigned int>
                         struct_serializer_simple(const struct Dummy<SIZE>& v) {
    std::unique_ptr<char[]> ptr(new char[sizeof(struct Dummy<SIZE>)]);
    std::memcpy(ptr.get(), &v, sizeof(struct Dummy<SIZE>));
    return std::make_pair(std::move(ptr), sizeof(struct Dummy<SIZE>));
}

template<unsigned int SIZE>
/* Takes a pointer to struct Dummy passed in and returns as object. */
struct Dummy<SIZE> struct_deserializer_simple(void* data,
                                              unsigned int /* size */) {
    struct Dummy<SIZE> *ptr = static_cast<struct Dummy<SIZE>*>(data);
    struct Dummy<SIZE> retDummy(ptr->id);
    std::memcpy(&retDummy.data, &(ptr->data), SIZE);
    return retDummy;
}

/* This function simply copies an int into a new portion of memory. */
std::pair<std::unique_ptr<char[]>, unsigned int>
    serializer_simple(const int& v) {

    std::unique_ptr<char[]> ptr(new char[sizeof(int)]);
    *(ptr.get()) = v;
    return std::make_pair(std::move(ptr), sizeof(int));
}

/* Takes an int passed in and returns as object. */
int deserializer_simple(void* data, unsigned int /* size */) {
    int *ptr = reinterpret_cast<int *>(data);
    int retval = *ptr;
    return retval;
}

}  // namespace cirrus

#endif  // _OBJECT_STORE_INTERNAL_H_
