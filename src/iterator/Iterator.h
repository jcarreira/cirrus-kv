/* Copyright 2016 Joao Carreira */

#ifndef _ITERATOR_H_
#define _ITERATOR_H_

#include "src/cache_manager/CacheManager.h"

namespace cirrus {
using ObjectID = uint64_t;

/**
  * A class that interfaces with the cache manager. Making an access will
  * prefetch the next object
  */
template<class T>
class CacheManager {
 public:

 private:

};


}  // namespace cirrus

#endif  // _ITERATOR_H_
