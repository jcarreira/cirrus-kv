#ifndef SRC_OBJECT_STORE_OBJECTSTORE_H_
#define SRC_OBJECT_STORE_OBJECTSTORE_H_

#include <cstdint>

namespace cirrus {

using ObjectID = uint64_t;
using Object = void*;

template<typename T>
class ObjectStore {
 public:
    ObjectStore() {}

    virtual T get(const ObjectID& id) const = 0;

    virtual bool put(const ObjectID& id, const T& obj) = 0;

    virtual void printStats() const noexcept = 0;

    virtual bool remove(ObjectID id) = 0;

    // TODO(Tyler): Add async operations
 private:
};

}  // namespace cirrus

#endif  // SRC_OBJECT_STORE_OBJECTSTORE_H_
