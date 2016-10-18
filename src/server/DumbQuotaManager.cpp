/* Copyright 2016 Joao Carreira */

#include "DumbQuotaManager.h"

namespace sirius {

DumbQuotaManager::DumbQuotaManager() {

}
    
bool DumbQuotaManager::canAllocateMemory(uint64_t app_id, uint64_t memorySize) {
    return true;
}

bool DumbQuotaManager::allocateMemory(uint64_t app_id, uint64_t memorySize) {
    if (apps_to_mem.find(app_id) == apps_to_mem.end())
            return false;

    apps_to_mem[app_id] += memorySize;
    return true;
}

} // sirius

