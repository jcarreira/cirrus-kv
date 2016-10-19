/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_CLIENT_H_
#define _BLADE_CLIENT_H_

#include "src/client/RDMAClient.h"
#include <memory>
#include "src/common/AllocationRecord.h"
#include "src/authentication/AuthenticationToken.h"

namespace sirius {

typedef std::shared_ptr<AllocationRecord> AllocRec;

class BladeClient : public RDMAClient {
public:
    BladeClient(int timeout_ms = 500);
    ~BladeClient();

    bool authenticate(std::string allocator_address,
        std::string port, AuthenticationToken& auth_token);

    // allocate data
    // return 0 if error
    AllocRec allocate(uint64_t size);

    bool write(AllocRec alloc_rec, uint64_t offset, 
            uint64_t length, const void* data);
    bool write_sync(AllocRec alloc_rec, uint64_t offset, 
            uint64_t length, const void* data);
    bool read(AllocRec alloc_rec, uint64_t offset,
            uint64_t length, void *data);
    bool read_sync(AllocRec alloc_rec, uint64_t offset,
            uint64_t length, void *data);

private:
    uint64_t remote_addr_;
};

} // sirius

#endif // _BLADE_CLIENT_H_
