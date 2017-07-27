#ifndef SRC_ITERATOR_CIRRUSITERABLE_H_
#define SRC_ITERATOR_CIRRUSITERABLE_H_

#include <time.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
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
    /**
     * This enum defines different prefetch modes for the iterable cache.
     * The default is ordered prefetching.
     */
    enum PrefetchMode {
        kOrdered = 0, /**< The iterator will prefetch a few items ahead. */
        kUnOrdered /**< The cache will iterate randomly. */
    };

    CirrusIterable<T>::Iterator begin();
    CirrusIterable<T>::Iterator end();
    void setMode(PrefetchMode mode_);

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
                                    ObjectID last, ObjectID current_id,
                                    PrefetchMode mode);
        Iterator(const Iterator& it);

        T operator*();
        Iterator& operator++();
        Iterator operator++(int i);
        bool operator!=(const Iterator& it) const;
        bool operator==(const Iterator& it) const;
        ObjectID get_curr_id() const;

     private:
        /** Vector used to track ObjectIDs in unordered mode. */
        std::vector<ObjectID> id_vector;

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

        /** The mode that this iterator is operating in. */
        PrefetchMode mode;
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

    /** The mode that this iterator is operating in. */
    PrefetchMode mode = PrefetchMode::kOrdered;
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
 * Changes the iteration mode of the iterable cache.
 * @param mode_ the desired prefetching mode.
 */
template<class T>
void CirrusIterable<T>::setMode(CirrusIterable::PrefetchMode mode_) {
    switch (mode_) {
      case CirrusIterable::PrefetchMode::kOrdered: {
        mode = mode_;
        break;
      }
      case CirrusIterable::PrefetchMode::kUnOrdered: {
        mode = mode_;
        break;
      }
      default: {
        throw cirrus::Exception("Unrecognized prefetch mode");
      }
    }
}
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
  * @param mode the prefetching mode that this iterator should operate using.
  */
template<class T>
CirrusIterable<T>::Iterator::Iterator(cirrus::CacheManager<T>* cm,
                            unsigned int readAhead, ObjectID first,
                            ObjectID last, ObjectID current_id,
                            PrefetchMode mode):
                            cm(cm), readAhead(readAhead), first(first),
                            last(last), current_id(current_id), mode(mode) {
    // Set up id_vector if in unordered mode
    if (mode == CirrusIterable<T>::PrefetchMode::kUnOrdered) {
        for (int i = first; i <= last; i++) {
            id_vector.push_back(i);
        }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    shuffle(id_vector.begin(), id_vector.end(),
            std::default_random_engine(seed));
    }
}

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
    return CirrusIterable<T>::Iterator(cm, readAhead, first, last, first, mode);
}

/**
  * Function that returns a cirrus::Iterator one past the end of the given
  * range.
  */
template<class T>
typename CirrusIterable<T>::Iterator CirrusIterable<T>::end() {
    return CirrusIterable<T>::Iterator(cm, readAhead, first, last, last + 1,
                mode);
}

/**
  * Function that dereferences the iterator, retrieving the underlying object
  * of type T. When dereferenced, it prefetches the next readAhead items.
  * @return Returns an object of type T.
  */
template<class T>
T CirrusIterable<T>::Iterator::operator*() {
    // Attempts to get the next readAhead items.
    if (current_id > last) {
        throw cirrus::Exception("Attempting to dereference the "
            ".end() iterator");
    }

    if (mode == CirrusIterable<T>::PrefetchMode::kOrdered) {
        for (unsigned int i = 1; i <= readAhead; i++) {
            // Formula is
            // val = ((current_id + i) - first) % (last - first + 1)) + first
            // calculate what we WOULD fetch
            ObjectID tentative_fetch = current_id + i;
            // shift relative to first
            ObjectID shifted = tentative_fetch - first;
            // Mod relative to shifted last
            ObjectID modded = shifted % (last - first + 1);
            // Add back to first for final result
            ObjectID to_fetch = modded + first;
            cm->prefetch(to_fetch);
        }
        return cm->get(current_id);
    } else if (mode == CirrusIterable<T>::PrefetchMode::kUnOrdered) {
        // Code for unordered accesses goes here
        for (unsigned int i = 1; i <= readAhead; i++) {
            ObjectID tentative_fetch = current_id + i;
            if (tentative_fetch <= last) {
                cm->prefetch(id_vector[tentative_fetch - first]);
            }
        }
        // We are attempting to access index current_id - first
        return cm->get(id_vector[current_id - first]);
    } else {
        throw cirrus::Exception("Unrecognized prefetch mode");
    }
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

#endif  // SRC_ITERATOR_CIRRUSITERABLE_H_
