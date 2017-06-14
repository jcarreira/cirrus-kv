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
    cirrus::Iterator<T> begin();
    cirrus::Iterator<T> end();

    /**
      * Constructor for the CirrusIterable class. Assumes that all objects
      * are stored sequentially between first and last.
      * @param cm a pointer to a CacheManager with that contains the same
      * object type as this Iterable.
      * @param first the first sequential objectID. Should always be <= than
      * last.
      * @param the last sequential id under which an object is stored. Should
      * always be >= first.
      * @param readAhead how many items ahead items should be prefetched.
      * Should always be <= last - first.
      */
    CirrusIterable<T>(cirrus::CacheManager<T>* cm,
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

/**
  * Function that returns a cirrus::Iterator at the start of the given range.
  */
template<class T>
cirrus::Iterator<T> CirrusIterable<T>::begin() {
    return new cirrus::Iterator<T>(cm, readAhead, first, last, first);
}

/**
  * Function that returns a cirrus::Iterator one past the end of the given
  * range.
  */
template<class T>
cirrus::Iterator<T> CirrusIterable<T>::end() {
  return new cirrus::Iterator<T>(cm, readAhead, first, last, last + 1);
}

}  // namespace cirrus

#endif  // _CIRRUS_ITERABLE_H_
