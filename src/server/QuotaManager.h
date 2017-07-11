#ifndef SRC_SERVER_QUOTAMANAGER_H_
#define SRC_SERVER_QUOTAMANAGER_H_

#include <cstdint>

namespace cirrus {

class QuotaManager {
 public:
    QuotaManager();

    virtual bool canAllocateMemory(uint64_t app_id, uint64_t memorySize) = 0;;
    virtual void allocateMemory(uint64_t app_id, uint64_t memorySize) = 0;
};

}  // namespace cirrus

#endif  // SRC_SERVER_QUOTAMANAGER_H_
