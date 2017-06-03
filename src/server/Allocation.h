/* Copyright 2016 Joao Carreira */

#ifndef _ALLOCATION_H_
#define _ALLOCATION_H_



namespace cirrus {
/**
  * This base class describes a resource
  * reserved by one app.
  */
class Allocation {
public:
    Allocation() {}
    virtual ~Allocation() {}
private:

};

} // cirrus

#endif // _ALLOCATION_H_
