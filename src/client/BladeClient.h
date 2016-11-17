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

using AllocRec = std::shared_ptr<AllocationRecord>;
using OpRet = std::pair<bool, FutureBladeOp*>;

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
    ~BladeClient();

    bool authenticate(std::string allocator_address,
        std::string port, AuthenticationToken& auth_token);

    AllocRec allocate(uint64_t size);

    OpRet write_async(const AllocRec& alloc_rec,
            uint64_t offset, uint64_t length, const void* data);
    bool write_sync(const AllocRec& alloc_rec, uint64_t offset, 
            uint64_t length, const void* data);
    OpRet read_async(const AllocRec& alloc_rec,
            uint64_t offset, uint64_t length, void *data);
    bool read_sync(const AllocRec& alloc_rec, uint64_t offset,
            uint64_t length, void *data);

private:
    uint64_t remote_addr_;
};

} // sirius

#endif // _BLADE_CLIENT_H_
