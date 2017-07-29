#ifndef SRC_ITERATOR_CIRRUSITERABLE_H_
#define SRC_ITERATOR_CIRRUSITERABLE_H_

#include <time.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include "cache_manager/CacheManager.h"
#include "iterator/IteratorPolicy.h"

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
        kUnOrdered, /**< The cache will iterate randomly. */
        kCustom /**< The iterator will use a custom user policy. */
    };

    CirrusIterable<T>::Iterator begin();
    CirrusIterable<T>::Iterator end();
    void setMode(PrefetchMode mode_,
        IteratorPolicy* policy_ = nullptr);

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
                                    unsigned int read_ahead, ObjectID first,
                                    ObjectID last, bool is_begin,
                                    bool is_end,
                                    IteratorPolicy *policy);
        Iterator(const Iterator& it);

        T operator*();
        Iterator& operator++();
        Iterator operator++(int i);
        bool operator!=(const Iterator& it) const;
        bool operator==(const Iterator& it) const;

     private:
        /**
         * Pointer to CacheManager used for put, get, and prefetch.
         */
        cirrus::CacheManager<T> *cm;

        std::unique_ptr<IteratorPolicy> policy;
    };

    class OrderedPolicy : public IteratorPolicy {
     public:
        OrderedPolicy() {}
        OrderedPolicy(const OrderedPolicy& other): first(other.first),
            last(other.last), read_ahead(other.read_ahead),
            current_id(other.current_id) {}

        void SetState(ObjectID first_, ObjectID last_, uint64_t read_ahead_,
            Position position_) override {
            first = first_;
            last = last_;
            read_ahead = read_ahead_;

            if (position_ == kBegin) {
                current_id = first;
            } else if (position_ == kEnd) {
                current_id = last + 1;
            } else {
                throw cirrus::Exception("Unrecognized position argument.");
            }
        }
        std::vector<ObjectID> GetPrefetchList() override {
            std::vector<ObjectID> prefetch_vector;
            for (unsigned int i = 1; i <= read_ahead; i++) {
                ObjectID tentative_fetch = current_id + i;
                ObjectID shifted = tentative_fetch - first;
                ObjectID modded = shifted % (last - first + 1);
                ObjectID to_fetch = modded + first;
                prefetch_vector.push_back(to_fetch);
            }
            return prefetch_vector;
        }

        ObjectID Dereference() override {
            return current_id;
        }

        void Increment() override {
            current_id++;
        }

        uint64_t GetState() override {
            return current_id;
        }

        std::unique_ptr<IteratorPolicy> Clone() override {
            return std::make_unique<OrderedPolicy>(*this);
        }

     private:
        ObjectID first;
        ObjectID last;
        uint64_t read_ahead;
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
    OrderedPolicy ordered;
    /** 
     * A pointer to the policy that will be used for the iterator. 
     * ordered by default.
     */
    IteratorPolicy *policy = &ordered;
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
 * @param policy_ a pointer to an IteratorPolicy that will be used if in custom
 * mode.
 */
template<class T>
void CirrusIterable<T>::setMode(CirrusIterable::PrefetchMode mode_,
        IteratorPolicy *policy_) {
    switch (mode_) {
      case CirrusIterable::PrefetchMode::kOrdered: {
        policy = &ordered;
        break;
      }
      case CirrusIterable::PrefetchMode::kUnOrdered: {
        // policy = &unordered;
        break;
      }
      case CirrusIterable::PrefetchMode::kCustom: {
        if (policy_ == nullptr) {
            throw cirrus::Exception("Custom prefetching specified without "
                    " a pointer to a custom policy.");
        }
        policy = policy_;
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
  * @param read_ahead how many items ahead items should be prefetched.
  * @param first the first sequential objectID. Should always be <= than
  * last.
  * @param last the last sequential id under which an object is stored. Should
  * always be >= first.
  * @param is_begin a boolean indicating if this will be a begin iterator
  * @param is_end a boolean indicating if this will be an end iterator
  * @param policy_ptr a pointer to an IteratorPolicy that will be used for
  *  all logic.
  */
template<class T>
CirrusIterable<T>::Iterator::Iterator(cirrus::CacheManager<T>* cm,
                            unsigned int read_ahead, ObjectID first,
                            ObjectID last, bool is_begin, bool is_end,
                            IteratorPolicy* policy_ptr):
                            cm(cm), policy(policy_ptr->Clone()) {
    IteratorPolicy::Position position;
    if (is_begin) {
        position = IteratorPolicy::Position::kBegin;
    } else if (is_end) {
        position = IteratorPolicy::Position::kEnd;
    } else {
        throw cirrus::Exception("Iterator constructor called "
                "with begin and end false.");
    }
    policy->SetState(first, last, read_ahead, position);
}

/**
 * Copy constructor for the Iterator class.
 */
template<class T>
CirrusIterable<T>::Iterator::Iterator(const Iterator& it):
              cm(it.cm), policy(it.policy->Clone()) {}

/**
 * Function that returns a cirrus::Iterator at the start of the given range.
 */
template<class T>
typename CirrusIterable<T>::Iterator CirrusIterable<T>::begin() {
    return CirrusIterable<T>::Iterator(cm, readAhead, first, last, true,
            false, policy);
}

/**
 * Function that returns a cirrus::Iterator one past the end of the given
 * range.
 */
template<class T>
typename CirrusIterable<T>::Iterator CirrusIterable<T>::end() {
    return CirrusIterable<T>::Iterator(cm, readAhead, first, last, false,
            true, policy);
}

/**
 * Function that dereferences the iterator, retrieving the underlying object
 * of type T. When dereferenced, it prefetches the next readAhead items.
 * @return Returns an object of type T.
 */
template<class T>
T CirrusIterable<T>::Iterator::operator*() {
    // Attempts to get the next readAhead items.
    auto to_prefetch = policy->GetPrefetchList();
    for (const auto& oid : to_prefetch) {
        cm->prefetch(oid);
    }

    cm->get(policy->Dereference());
}

/**
 * A function that increments the Iterator by increasing the value of
 * current_id. The next time the Iterator is dereferenced, an object stored
 * under the incremented current_id will be retrieved. Serves as preincrement.
 */
template<class T>
typename CirrusIterable<T>::Iterator&
CirrusIterable<T>::Iterator::operator++() {
    policy->Increment();
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
    return !(*this == it);
}

/**
 * A function that compares two Iterators. Will return true if the two
 * iterators have identical values of current_id.
 */
template<class T>
bool CirrusIterable<T>::Iterator::operator==(
                                 const CirrusIterable<T>::Iterator& it) const {
    return policy->GetState() == it.policy->GetState();
}

}  // namespace cirrus

#endif  // SRC_ITERATOR_CIRRUSITERABLE_H_
