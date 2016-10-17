/* Copyright 2016 Joao Carreira */

#ifndef _APPLICATION_RECORD_H_
#define _APPLICATION_RECORD_H_

namespace sirius {

class ApplicationRecord {
public:
    ApplicationRecord() {}
    virtual ~ApplicationRecord() {}
private:

    uint64_t app_id_;
    ApplicationQuota app_quota_;
    std::vector<Allocation> app_allocations;
};

} // sirius

#endif // _APPLICATION_RECORD_H_

