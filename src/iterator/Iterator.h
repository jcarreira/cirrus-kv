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
class Iterator {
 public:
   /**
     * Constructor for the Iterator class. Assumes that all objects
     * are stored sequentially.
     * @param cm a pointer to a CacheManager with that contains the same
     * object type as this Iterable.
     * @param current_id the id that will be fetched when the iterator is
     * dereferenced.
     * @param readAhead how many items ahead items should be prefetched.
     */
    Iterator(cirrus::CacheManager<T>* cm,
                                unsigned int readAhead, ObjectID first,
                                ObjectID last, ObjectID current_id):
                                cm(cm), readAhead(readAhead), first(first),
                                last(last), current_id(current_id) {}

    T operator*();
    Iterator& operator++();
    Iterator& operator++(int i);
    bool operator!=(const Iterator& it) const;
    bool operator==(const Iterator& it) const;
    ObjectID get_curr_id() const;

 private:
    cirrus::CacheManager<T> *cm;
    unsigned int readAhead;
    ObjectID first;
    ObjectID last;
    ObjectID current_id;
};

template<class T>
T Iterator<T>::operator*() {
  for (unsigned int i = 1; i <= readAhead; i++) {
    // Math to make sure that prefetching loops back around
    // Formula is val = ((current_id + i) - first) % (last - first)) + first
    ObjectID tenative_fetch = current_id + i;  // calculate what we WOULD fetch
    ObjectID shifted = tenative_fetch - first;  // shift relative to first
    ObjectID modded = shifted % (last - first);  // Mod relative to shifted last
    ObjectID to_fetch = modded + first;  // Add back to first for final result
    cm->prefetch(to_fetch);
  }

  return cm->get(current_id);
}

template<class T>
Iterator<T>& Iterator<T>::operator++() {
  current_id++;
  return *this;
}

template<class T>
Iterator<T>& Iterator<T>::operator++(int /* i */) {
  current_id++;
  return *this;
}

template<class T>
bool Iterator<T>::operator!=(const Iterator<T>& it) const {
  return current_id != it.get_curr_id();
}

template<class T>
bool Iterator<T>::operator==(const Iterator<T>& it) const {
  return current_id == it.get_curr_id();
}

template<class T>
ObjectID Iterator<T>::get_curr_id() const {
  return current_id;
}
}  // namespace cirrus

#endif  // _ITERATOR_H_
