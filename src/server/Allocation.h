/* Copyright 2016 Joao Carreira */

#ifndef _ALLOCATION_H_
#define _ALLOCATION_H_

/*
 * This base class describes a resource
 * reserved by one app
 */

namespace sirius {

class Allocation {
public:
    Allocation() {}
    virtual ~Allocation() {}
private:

};

} // sirius

#endif // _ALLOCATION_H_

