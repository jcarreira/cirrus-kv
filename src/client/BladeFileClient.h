/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_FILE_CLIENT_H_
#define _BLADE_FILE_CLIENT_H_

#include "src/client/RDMAClient.h"
#include <memory>
#include "src/common/FileAllocationRecord.h"
#include "src/authentication/AuthenticationToken.h"


// XXX hack. We use the FutureBladeOp. Need to refactor this
#include "src/client/BladeClient.h"

namespace cirrus {

using FileAllocRec = FileAllocationRecord;

class BladeFileClient : public RDMAClient {
public:
    BladeFileClient(int timeout_ms = 500);
    ~BladeFileClient() = default;

    bool authenticate(std::string allocator_address,
        std::string port, AuthenticationToken& auth_token);

    FileAllocRec allocate(const std::string& filename, uint64_t size);

    bool write_sync(const FileAllocRec& alloc_rec, uint64_t offset, 
            uint64_t length, const void* data);
    std::shared_ptr<FutureBladeOp> write_async(const FileAllocRec& alloc_rec, uint64_t offset, 
            uint64_t length, const void* data, RDMAMem&);


    bool read_sync(const FileAllocRec& alloc_rec, uint64_t offset,
            uint64_t length, void *data);
    std::shared_ptr<FutureBladeOp> read_async(const FileAllocRec& alloc_rec, uint64_t offset,
            uint64_t length, const void* data, RDMAMem&);

private:
    uint64_t remote_addr_;
};

} // cirrus

#endif // _BLADE_FILE_CLIENT_H_
