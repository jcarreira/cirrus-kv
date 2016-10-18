/* Copyright 2016 Joao Carreira */

#ifndef _QUOTA_MANAGER_H_
#define _QUOTA_MANAGER_H_

#include <cstdint>

namespace sirius {

class QuotaManager {
public:
    QuotaManager();

    virtual bool canAllocateMemory(uint64_t app_id, uint64_t memorySize) = 0;;
    virtual void allocateMemory(uint64_t app_id, uint64_t memorySize) = 0;
};

} // sirius

#endif // _QUOTA_MANAGER_H_
