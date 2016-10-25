/* Copyright 2016 Joao Carreira */

#ifndef _OBJECT_STORE_H_
#define _OBJECT_STORE_H_

#include <cstdint>

namespace sirius {

typedef uint64_t ObjectName;
typedef void* Object;

class ObjectStore {
public:
    ObjectStore();

    virtual Object get(ObjectName) = 0;
    virtual bool put(Object, ObjectName) = 0;

private:
};

}

#endif // _OBJECT_STORE_H_
