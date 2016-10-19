/* Copyright 2016 Joao Carreira */

#include "src/server/ApplicationRecord.h"

namespace sirius {

ApplicationRecord::ApplicationRecord(uint64_t app_id) :
    app_id_(app_id) {
}

ApplicationRecord::~ApplicationRecord() {
}

void ApplicationRecord::addAllocation(const Allocation& alloc) {
    allocations_.push_back(alloc);
}

void ApplicationRecord::setQuota(const ApplicationQuota& quota) {
    quota_ = quota;
}

}  // namespace sirius
