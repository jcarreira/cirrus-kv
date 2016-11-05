/* Copyright 2016 Joao Carreira */

#ifndef _FILE_ALLOCATION_RECORD_H_
#define _FILE_ALLOCATION_RECORD_H_

namespace sirius {

struct FileAllocationRecord {
    uint64_t remote_addr;
    uint64_t peer_rkey;

    FileAllocationRecord(uint64_t remote_addr_, uint64_t peer_rkey_) :
        remote_addr(remote_addr_), peer_rkey(peer_rkey_)
    {}
};

}

#endif // _FILE_ALLOCATION_RECORD_H_
