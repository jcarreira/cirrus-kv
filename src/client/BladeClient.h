/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_CLIENT_H_
#define _BLADE_CLIENT_H_

#include "src/client/RDMAClient.h"
#include <memory>
#include <utility>
#include "src/common/AllocationRecord.h"
#include "src/authentication/AuthenticationToken.h"

namespace sirius {

class FutureBladeOp;

class FutureBladeOp {
public:
    FutureBladeOp(RDMAOpInfo* info) :
        op_info(info) {}
    
    virtual ~FutureBladeOp();

    void wait();
    bool try_wait();

private: 
    RDMAOpInfo* op_info;
};

class BladeClient : public RDMAClient {
public:
    BladeClient(int timeout_ms = 500);
    ~BladeClient() = default;

    bool authenticate(std::string allocator_address,
        std::string port, AuthenticationToken& auth_token);

    AllocationRecord allocate(uint64_t size);

    // writes
    std::shared_ptr<FutureBladeOp> write_async(const AllocationRecord& alloc_rec,
            uint64_t offset, uint64_t length, const void* data,
            RDMAMem* mem = nullptr);
    bool write_sync(const AllocationRecord& alloc_rec, uint64_t offset, 
            uint64_t length, const void* data, RDMAMem* mem = nullptr);

    // reads
    std::shared_ptr<FutureBladeOp> read_async(const AllocationRecord& alloc_rec,
            uint64_t offset, uint64_t length, void *data,
            RDMAMem* mem = nullptr);
    bool read_sync(const AllocationRecord& alloc_rec, uint64_t offset,
            uint64_t length, void *data, RDMAMem* reg = nullptr);

private:
    uint64_t remote_addr_;
};

} // sirius

#endif // _BLADE_CLIENT_H_
