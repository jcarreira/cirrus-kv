#ifndef _CIRRUS_ITERABLE_H_
#define _CIRRUS_ITERABLE_H_

#include "cache_manager/CacheManager.h"

namespace cirrus {
using ObjectID = uint64_t;

/**
  * A class that interfaces with the cache manager. Returns cirrus::Iterator
  * objects for iteration.
  */
template<class T>
class CirrusIterable {
    class Iterator;
 public:
    CirrusIterable<T>::Iterator begin();
    CirrusIterable<T>::Iterator end();

    CirrusIterable<T>(cirrus::CacheManager<T>* cm,
                                 unsigned int readAhead,
                                 ObjectID first,
                                 ObjectID last);

 private:
    /**
      * A class that interfaces with the cache manager. Making an access will
      * prefetch a user defined distance ahead
      */
    class Iterator {
     public:
        Iterator(cirrus::CacheManager<T>* cm,
                                    unsigned int readAhead, ObjectID first,
                                    ObjectID last, ObjectID current_id);
        Iterator(const Iterator& it);

        T operator*();
        Iterator& operator++();
        Iterator operator++(int i);
        bool operator!=(const Iterator& it) const;
        bool operator==(const Iterator& it) const;
        ObjectID get_curr_id() const;

     private:
        /**
          * Pointer to CacheManager used for put, get, and prefetch.
          */
        cirrus::CacheManager<T> *cm;

        /**
          * How many items ahead to prefetch on a dereference.
          */
        unsigned int readAhead;

        /**
          * First sequential ID.
          */
        ObjectID first;

        /**
          * Last sequential ID.
          */
        ObjectID last;

        /**
          * The ObjectID that will be get() will be called on the
          * next time the iterator is dereferenced.
          */
        ObjectID current_id;
    };

    /**
      * Pointer to CacheManager used for put, get, and prefetch.
      */
    cirrus::CacheManager<T> *cm;

    /**
      * How many items ahead to prefetch on a dereference.
      */
    unsigned int readAhead;

    /**
      * First sequential ID.
      */
    ObjectID first;
    
    /**
      * Last sequential ID.
      */
    ObjectID last;
};

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
  * Should always be <= last - first. Additionally, should be less than
  * the cache capacity that was specified in the creation of the
  * CacheManager.
  */
template<class T>
CirrusIterable<T>::CirrusIterable(cirrus::CacheManager<T>* cm,
                             unsigned int readAhead,
                             ObjectID first,
                             ObjectID last):
                             cm(cm), readAhead(readAhead), first(first),
                             last(last) {}

/**
  * Constructor for the Iterator class. Assumes that all objects
  * are stored sequentially.
  * @param cm a pointer to a CacheManager with that contains the same
  * object type as this Iterable.
  * @param readAhead how many items ahead items should be prefetched.
  * @param first the first sequential objectID. Should always be <= than
  * last.
  * @param the last sequential id under which an object is stored. Should
  * always be >= first.
  * @param current_id the id that will be fetched when the iterator is
  * dereferenced.
  */
template<class T>
CirrusIterable<T>::Iterator::Iterator(cirrus::CacheManager<T>* cm,
                            unsigned int readAhead, ObjectID first,
                            ObjectID last, ObjectID current_id):
                            cm(cm), readAhead(readAhead), first(first),
                            last(last), current_id(current_id) {}

/**
  * Copy constructor for the Iterator class.
  */
template<class T>
CirrusIterable<T>::Iterator::Iterator(const Iterator& it):
              cm(it.cm), readAhead(it.readAhead), first(it.first),
              last(it.last), current_id(it.current_id) {}

/**
  * Function that returns a cirrus::Iterator at the start of the given range.
  */
template<class T>
typename CirrusIterable<T>::Iterator CirrusIterable<T>::begin() {
    return CirrusIterable<T>::Iterator(cm, readAhead, first, last, first);
}

/**
  * Function that returns a cirrus::Iterator one past the end of the given
  * range.
  */
template<class T>
typename CirrusIterable<T>::Iterator CirrusIterable<T>::end() {
  return CirrusIterable<T>::Iterator(cm, readAhead, first, last, last + 1);
}

/**
  * Function that dereferences the iterator, retrieving the underlying object
  * of type T. When dereferenced, it prefetches the next readAhead items.
  * @return Returns an object of type T.
  */
template<class T>
T CirrusIterable<T>::Iterator::operator*() {
  // Attempts to get the next readAhead items.
  for (unsigned int i = 1; i <= readAhead; i++) {
    // Math to make sure that prefetching loops back around
    // Formula is val = ((current_id + i) - first) % (last - first)) + first
    ObjectID tentative_fetch = current_id + i;  // calculate what we WOULD fetch
    ObjectID shifted = tentative_fetch - first;  // shift relative to first
    ObjectID modded = shifted % (last - first);  // Mod relative to shifted last
    ObjectID to_fetch = modded + first;  // Add back to first for final result
    cm->prefetch(to_fetch);
  }

  return cm->get(current_id);
}

/**
  * A function that increments the Iterator by increasing the value of
  * current_id. The next time the Iterator is dereferenced, an object stored
  * under the incremented current_id will be retrieved. Serves as preincrement.
  */
template<class T>
typename CirrusIterable<T>::Iterator&
CirrusIterable<T>::Iterator::operator++() {
  current_id++;
  return *this;
}


/**
  * A function that increments the Iterator by increasing the value of
  * current_id. The next time the Iterator is dereferenced, an object stored
  * under the incremented current_id will be retrieved. Serves as post
  * increment.
  */
template<class T>
typename CirrusIterable<T>::Iterator CirrusIterable<T>::Iterator::operator++(
                                                                int /* i */) {
  typename CirrusIterable<T>::Iterator tmp(*this);
  operator++();
  return tmp;
}

/**
  * A function that compares two Iterators. Will return true if the two
  * iterators have different values of current_id.
  */
template<class T>
bool CirrusIterable<T>::Iterator::operator!=(
                               const CirrusIterable<T>::Iterator& it) const {
  return current_id != it.get_curr_id();
}

/**
  * A function that compares two Iterators. Will return true if the two
  * iterators have identical values of current_id.
  */
template<class T>
bool CirrusIterable<T>::Iterator::operator==(
                                 const CirrusIterable<T>::Iterator& it) const {
  return current_id == it.get_curr_id();
}

/**
  * A function that returns the current_id of the Iterator that calls it.
  */
template<class T>
ObjectID CirrusIterable<T>::Iterator::get_curr_id() const {
  return current_id;
}


}  // namespace cirrus

#endif  // _CIRRUS_ITERABLE_H_
