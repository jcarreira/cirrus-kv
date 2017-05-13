/* Copyright 2016 Joao Carreira */

#ifndef _APPLICATION_RECORD_H_
#define _APPLICATION_RECORD_H_

namespace cirrus {

class ApplicationRecord {
public:
    ApplicationRecord();
    virtual ~ApplicationRecord();

    void addAllocation(const Allocation& alloc);
    void setQuota(const ApplicationQuota& quota);

private:
    uint64_t app_id_;
    ApplicationQuota quota_;
    std::vector<Allocation> allocations_;
};

} // cirrus

#endif // _APPLICATION_RECORD_H_

