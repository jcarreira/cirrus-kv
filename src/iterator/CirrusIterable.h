/* Copyright 2016 Joao Carreira */

#ifndef _CIRRUS_ITERABLE_H_
#define _CIRRUS_ITERABLE_H_

#include "src/cache_manager/CacheManager.h"
#include "src/iterator/Iterator.h"

namespace cirrus {
using ObjectID = uint64_t;

/**
  * A class that interfaces with the cache manager. Making an access will
  * prefetch the next object.
  */
template<class T>
class CirrusIterable {
 public:
    cirrus::Iterator<T> cirrus::CirrusIterable::begin();
    cirrus::Iterator<T> cirrus::CirrusIterable::end();

    /**
      * Constructor for the CirrusIterable class. Assumes that all objects
      * are stored sequentially between first and last.
      * @param cm a pointer to a CacheManager with that contains the same
      * object type as this Iterable.
      * @param first the first sequential objectID.
      * @param the last sequential id under which an object is stored.
      * @param readAhead how many items ahead items should be prefetched.
      */
    cirrus::CirrusIterable<T>(cirrus::CacheManager<T>* cm,
                                 unsigned int readAhead,
                                 ObjectID first,
                                 ObjectID last):
                                 cm(cm), readAhead(readAhead), first(first),
                                 last(last) {}

 private:
    cirrus::CacheManager<T> *cm;
    unsigned int readAhead;
    ObjectID first;
    ObjectID last;
};

cirrus::Iterator<T> CirrusIterable<T>::begin() {

}

cirrus::Iterator<T> CirrusIterable<T>::end() {

}


}  // namespace cirrus

#endif  // _CIRRUS_ITERABLE_H_
