#ifndef SRC_ITERATOR_ITERATORPOLICY_H_
#define SRC_ITERATOR_ITERATORPOLICY_H_

#include <vector>
#include <memory>
#include "iterator/CirrusIterable.h"
namespace cirrus {

using ObjectID = uint64_t;

/**
 * A class that outlines the interface that PrefetchPolicies for the iterator
 * should use.
 */
class IteratorPolicy {
 public:
    enum Position {
        kBegin = 0,
        kEnd
    };

    virtual void setState(ObjectID first, ObjectID last, uint64_t read_ahead,
            Position position) = 0;
    /**
     * Called by the Iterator whenever it dereferences an object.
     * @param oid the ObjectID being dereferenced.
     * @param it a reference to the iterator that is making the call.
     * @return a std::vector of ObjectIDs that should be prefetched.
     */
    virtual std::vector<ObjectID> getPrefetchList() = 0;

    virtual ObjectID dereference() = 0;

    virtual void increment() = 0;

    virtual uint64_t getState() const = 0;

    virtual std::unique_ptr<IteratorPolicy> clone() const = 0;
};

}  // namespace cirrus

#endif  // SRC_ITERATOR_ITERATORPOLICY_H_
