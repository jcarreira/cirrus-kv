/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_FILE_CLIENT_H_
#define _BLADE_FILE_CLIENT_H_

#include "src/client/RDMAClient.h"
#include <memory>
#include "src/common/FileAllocationRecord.h"
#include "src/authentication/AuthenticationToken.h"

namespace sirius {

using FileAllocRec = std::shared_ptr<FileAllocationRecord>;

class BladeFileClient : public RDMAClient {
public:
    BladeFileClient(int timeout_ms = 500);
    ~BladeFileClient();

    bool authenticate(std::string allocator_address,
        std::string port, AuthenticationToken& auth_token);

    FileAllocRec allocate(const std::string& filename, uint64_t size);

    bool write(const FileAllocRec& alloc_rec, uint64_t offset, 
            uint64_t length, const void* data);
    bool write_sync(const FileAllocRec& alloc_rec, uint64_t offset, 
            uint64_t length, const void* data);
    bool read(const FileAllocRec& alloc_rec, uint64_t offset,
            uint64_t length, void *data);
    bool read_sync(const FileAllocRec& alloc_rec, uint64_t offset,
            uint64_t length, void *data);

private:
    uint64_t remote_addr_;
};

} // sirius

#endif // _BLADE_FILE_CLIENT_H_
