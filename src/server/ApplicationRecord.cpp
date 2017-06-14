#include "src/server/ApplicationRecord.h"

namespace cirrus {

ApplicationRecord::ApplicationRecord(uint64_t app_id) :
    app_id_(app_id) {
}

ApplicationRecord::~ApplicationRecord() {
}

void ApplicationRecord::addAllocation(const Allocation& alloc) {
    allocations_.push_back(alloc);
}

}  // namespace cirrus
