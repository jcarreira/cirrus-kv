/* Copyright 2016 Joao Carreira */

#ifndef _DUMB_QUOTA_MANAGER_H_
#define _DUMB_QUOTA_MANAGER_H_

#include "QuotaManager.h"
#include <map>

namespace sirius {

class DumbQuotaManager {
public:
    DumbQuotaManager() = default;
    
    bool canAllocateMemory(uint64_t app_id, uint64_t memorySize);
    bool allocateMemory(uint64_t app_id, uint64_t memorySize);

private:
    std::map<uint64_t, uint64_t> apps_to_mem;
};

} // sirius

#endif // _DUMB_QUOTA_MANAGER_H_
