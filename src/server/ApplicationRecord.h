#ifndef _APPLICATION_RECORD_H_
#define _APPLICATION_RECORD_H_

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

#endif  // _APPLICATION_RECORD_H_
