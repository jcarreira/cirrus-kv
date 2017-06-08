/* Copyright 2016 Joao Carreira */

#ifndef _OBJECT_STORE_H_
#define _OBJECT_STORE_H_

#include <cstdint>

namespace cirrus {

using ObjectID = uint64_t;
using Object = void*;

template<typename T>
class ObjectStore {
public:
    ObjectStore() {}

    virtual T get(const ObjectID&) const = 0;

//    virtual bool put(Object, uint64_t size, ObjectID) = 0;

    virtual void printStats() const noexcept = 0;

private:
};

}

#endif // _OBJECT_STORE_H_
