#ifndef SRC_SERVER_APPLICATIONRECORD_H_
#define SRC_SERVER_APPLICATIONRECORD_H_

#include <vector>

namespace cirrus {

class ApplicationRecord {
 public:
    ApplicationRecord();
    virtual ~ApplicationRecord();

    void addAllocation(const Allocation& alloc);

 private:
    uint64_t app_id_;
    std::vector<Allocation> allocations_;
};

}  // namespace cirrus

#endif  // SRC_SERVER_APPLICATIONRECORD_H_
