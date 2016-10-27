/* Copyright 2016 Joao Carreira */

#ifndef _OBJECT_STORE_H_
#define _OBJECT_STORE_H_

#include <cstdint>

namespace sirius {

typedef uint64_t ObjectID;
typedef void* Object;

class ObjectStore {
public:
    ObjectStore();

    virtual Object get(ObjectID) = 0;
    virtual bool put(Object, uint64_t size, ObjectID) = 0;
    virtual void printStats() = 0;

private:
};

}

#endif // _OBJECT_STORE_H_
