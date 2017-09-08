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
#include "utils/logging.h"


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
        kUnordered, /**< The cache will iterate randomly. */
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

        Iterator& operator=(const Iterator& other);

     private:
        /**
         * Pointer to CacheManager used for put, get, and prefetch.
         */
        cirrus::CacheManager<T> *cm;

        /**
         * Pointer to the IteratorPolicy that will be used to track all state.
         */
        std::unique_ptr<IteratorPolicy> policy;
    };
    /**
     * An IteratorPolicy that will traverse the specified range in order
     * from the lowest value to the highest. This range is only traversed once.
     */
    class OrderedPolicy : public IteratorPolicy {
     public:
        /**
         * Default constructor.
         */
        OrderedPolicy() : first(0), last(0), read_ahead(0), current_id(0) {}
        /**
         * Copy constructor, used by clone method.
         */
        OrderedPolicy(const OrderedPolicy& other): first(other.first),
            last(other.last), read_ahead(other.read_ahead),
            current_id(other.current_id) {}
        /**
         * Used to set the internal state of the policy after creation.
         * @param first_ the first ObjectID in a continuous range.
         * @param last_ the last ObjectID in the continuous range.
         * @param read_ahead_ how many items ahead the iterator shold prefetch.
         * @param position_ an enum indicating if this instance of the policy
         * should instantiate itself at the beginning or end of the range.
         */
        void setState(ObjectID first_, ObjectID last_, uint64_t read_ahead_,
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
        std::vector<ObjectID> getPrefetchList() override {
            std::vector<ObjectID> prefetch_vector;
            prefetch_vector.reserve(read_ahead);
            for (unsigned int i = 1; i <= read_ahead; i++) {
                ObjectID tentative_fetch = current_id + i;
                ObjectID shifted = tentative_fetch - first;
                ObjectID modded = shifted % (last - first + 1);
                ObjectID to_fetch = modded + first;
                if (to_fetch == current_id) {
                    break;
                }
                prefetch_vector.push_back(to_fetch);
            }
            return prefetch_vector;
        }
        /**
         * Returns the ObjectID at the current position.
         */
        ObjectID dereference() override {
            return current_id;
        }

        /**
         * Increments the internal state of the policy, moving it to the next
         * ObjectID to be returned.
         */
        void increment() override {
            current_id++;
        }

        /**
         * Returns the value of current_id, which is indicative of the state
         * of the policy.
         */
        uint64_t getState() const override {
            return current_id;
        }

        /**
         * An implementation of the clone method from the template.
         */
        std::unique_ptr<IteratorPolicy> clone() const override {
            return std::make_unique<OrderedPolicy>(*this);
        }

     private:
        /** First ObjectID in the continuous range. */
        ObjectID first;
        /** Last ObjectID in the continuous range. */
        ObjectID last;
        /** How many items ahead to prefetch. */
        uint64_t read_ahead;
        /** The ObjectID that will be returned when dereference is called. */
        ObjectID current_id;
    };

    /**
     * An IteratorPolicy that will traverse the specified range in a random
     * order. Each ObjectID is present only once in the pattern.
     */
    class UnorderedPolicy : public IteratorPolicy {
     public:
        /**
         * Default constructor.
         */
        UnorderedPolicy() = default;
        /**
         * Copy constructor, used by clone method.
         */
        explicit UnorderedPolicy(const OrderedPolicy& other):
            read_ahead(other.read_ahead),
            id_vector(other.id_vector), current_index(other.current_index) {}
        /**
         * Used to set the internal state of the policy after creation.
         * @param first_ the first ObjectID in a continuous range.
         * @param last_ the last ObjectID in the continuous range.
         * @param read_ahead_ how many items ahead the iterator shold prefetch.
         * @param position_ an enum indicating if this instance of the policy
         * should instantiate itself at the beginning or end of the range.
         */
        void setState(ObjectID first_, ObjectID last_, uint64_t read_ahead_,
            Position position_) override {
            read_ahead = read_ahead_;

            // Create a vector holding all the ObjectIDs to iterate over
            for (ObjectID i = first_; i <= last_; i++) {
                id_vector.push_back(i);
            }

            // Shuffle the ObjectIDs
            unsigned seed =
                std::chrono::system_clock::now().time_since_epoch().count();
            std::shuffle(id_vector.begin(), id_vector.end(),
                std::default_random_engine(seed));

            // Set the current index
            if (position_ == kBegin) {
                current_index = 0;
            } else if (position_ == kEnd) {
                current_index = last_ - first_ + 1;
            } else {
                throw cirrus::Exception("Unrecognized position argument.");
            }
        }

        /**
         * Returns the list of ObjectIDs to prefetch based on the current
         * internal state.
         */
        std::vector<ObjectID> getPrefetchList() override {
            std::vector<ObjectID> prefetch_vector;
            prefetch_vector.reserve(read_ahead);
            for (unsigned int i = 1; i <= read_ahead; i++) {
                if (current_index + i < id_vector.size()) {
                    prefetch_vector.push_back(id_vector[current_index + i]);
                }
            }
            return prefetch_vector;
        }
        /**
         * Returns the ObjectID at the current position.
         */
        ObjectID dereference() override {
            return id_vector[current_index];
        }

        /**
         * Increments the internal state of the policy, moving it to the next
         * ObjectID to be returned.
         */
        void increment() override {
            current_index++;
        }

        /**
         * Returns the value of current_id, which is indicative of the state
         * of the policy.
         */
        uint64_t getState() const override {
            return current_index;
        }

        /**
         * An implementation of the clone method from the template.
         */
        std::unique_ptr<IteratorPolicy> clone() const override {
            return std::make_unique<UnorderedPolicy>(*this);
        }

     private:
        /** How many items ahead to prefetch. */
        uint64_t read_ahead = 0;
        /** The current position within the vector of ObjectIDs to return. */
        uint64_t current_index = 0;
        /** Vector used to track the order to iterate in. */
        std::vector<ObjectID> id_vector;
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
    /** An ordered policy. */
    OrderedPolicy ordered;
    /** An unordered policy. */
    UnorderedPolicy unordered;
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
      case CirrusIterable::PrefetchMode::kUnordered: {
        policy = &unordered;
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
                            cm(cm), policy(policy_ptr->clone()) {
    IteratorPolicy::Position position;
    if (is_begin) {
        position = IteratorPolicy::Position::kBegin;
    } else if (is_end) {
        position = IteratorPolicy::Position::kEnd;
    } else {
        throw cirrus::Exception("Iterator constructor called "
                "with begin and end false.");
    }
    policy->setState(first, last, read_ahead, position);
}

/**
 * Copy constructor for the Iterator class.
 */
template<class T>
CirrusIterable<T>::Iterator::Iterator(const Iterator& it):
              cm(it.cm), policy(it.policy->clone()) {}

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
    LOG<INFO>("Call to dereference");

    TimerFunction tf;
    auto to_prefetch = policy->getPrefetchList();
    for (const auto& oid : to_prefetch) {
        LOG<INFO>("Prefetching oid: ", oid);
        cm->prefetch(oid);
    }

    auto ret = cm->get(policy->dereference());
    return ret;
}

/**
 * A function that increments the Iterator by increasing the value of
 * current_id. The next time the Iterator is dereferenced, an object stored
 * under the incremented current_id will be retrieved. Serves as preincrement.
 */
template<class T>
typename CirrusIterable<T>::Iterator&
CirrusIterable<T>::Iterator::operator++() {
    policy->increment();
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
    return policy->getState() != it.policy->getState();
}

/**
 * A function that compares two Iterators. Will return true if the two
 * iterators have identical values of current_id.
 */
template<class T>
bool CirrusIterable<T>::Iterator::operator==(
                                 const CirrusIterable<T>::Iterator& it) const {
    return policy->getState() == it.policy->getState();
}

template<class T>
typename CirrusIterable<T>::Iterator& CirrusIterable<T>::Iterator::operator=(
                               const CirrusIterable<T>::Iterator& other) {
    if (this != &other) {
        cm = other.cm;
        policy = other.policy->clone();
    }

    return *this;
}

}  // namespace cirrus

#endif  // SRC_ITERATOR_CIRRUSITERABLE_H_
