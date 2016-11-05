/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_CLIENT_H_
#define _BLADE_CLIENT_H_

#include "src/client/RDMAClient.h"
#include <memory>
#include "src/common/AllocationRecord.h"
#include "src/authentication/AuthenticationToken.h"

namespace sirius {

using AllocRec = std::shared_ptr<AllocationRecord>;

class BladeClient : public RDMAClient {
public:
    BladeClient(int timeout_ms = 500);
    ~BladeClient();

    bool authenticate(std::string allocator_address,
        std::string port, AuthenticationToken& auth_token);

    AllocRec allocate(uint64_t size);

    bool write(const AllocRec& alloc_rec, uint64_t offset, 
            uint64_t length, const void* data);
    bool write_sync(const AllocRec& alloc_rec, uint64_t offset, 
            uint64_t length, const void* data);
    bool read(const AllocRec& alloc_rec, uint64_t offset,
            uint64_t length, void *data);
    bool read_sync(const AllocRec& alloc_rec, uint64_t offset,
            uint64_t length, void *data);

private:
    uint64_t remote_addr_;
};

} // sirius

#endif // _BLADE_CLIENT_H_
