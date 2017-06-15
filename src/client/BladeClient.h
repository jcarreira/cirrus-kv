#ifndef _BLADE_CLIENT_H_
#define _BLADE_CLIENT_H_

#include <memory>
#include <utility>
#include <string>
#include "client/RDMAClient.h"
#include "common/AllocationRecord.h"
#include "authentication/AuthenticationToken.h"

namespace cirrus {

class FutureBladeOp;

/**
  * Information about an async op.
  * A class that contains information about an operation being performed
  * asynchronously.
  */
class FutureBladeOp {
 public:
    explicit FutureBladeOp(RDMAOpInfo* info) :
        op_info(info) {}

    virtual ~FutureBladeOp() = default;

    void wait();
    bool try_wait();

 private:
    /** op_info for this FutureBladeOp. */
    RDMAOpInfo* op_info;
};


/**
  * A class extending RDMAClient.
  * This class extends RDMAClient, providing implementations for many methods.
  */
class BladeClient : public RDMAClient {
 public:
    explicit BladeClient(int timeout_ms = 500);
    ~BladeClient() = default;

    bool authenticate(std::string allocator_address,
        std::string port, AuthenticationToken& auth_token);

    AllocationRecord allocate(uint64_t size);
    bool deallocate(const AllocationRecord& ar);

    // writes
    std::shared_ptr<FutureBladeOp> write_async(
            const AllocationRecord& alloc_rec,
            uint64_t offset, uint64_t length,
            const void* data,
            RDMAMem* mem = nullptr);
    bool write_sync(const AllocationRecord& alloc_rec, uint64_t offset,
            uint64_t length, const void* data, RDMAMem* mem = nullptr);

    // XXX We may not need a shared ptr here
    // reads
    std::shared_ptr<FutureBladeOp> read_async(const AllocationRecord& alloc_rec,
            uint64_t offset, uint64_t length, void *data,
            RDMAMem* mem = nullptr);
    bool read_sync(const AllocationRecord& alloc_rec, uint64_t offset,
            uint64_t length, void *data, RDMAMem* reg = nullptr);

    // fetch and add atomic
    std::shared_ptr<FutureBladeOp> fetchadd_async(
            const AllocationRecord& alloc_rec,
            uint64_t offset,
            uint64_t value);
    bool fetchadd_sync(const AllocationRecord& alloc_rec, uint64_t offset,
            uint64_t value);

 private:
    uint64_t remote_addr_;
};

}  // namespace cirrus

#endif  // _BLADE_CLIENT_H_
