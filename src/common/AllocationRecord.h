/* Copyright 2016 Joao Carreira */

#ifndef _ALLOCATION_RECORD_H_
#define _ALLOCATION_RECORD_H_

namespace sirius {

struct AllocationRecord {
    int alloc_id;
    uint64_t remote_addr;
    uint64_t peer_rkey;

    AllocationRecord(int alloc_id_, uint64_t remote_addr_, uint64_t peer_rkey_) :
        alloc_id(alloc_id_), remote_addr(remote_addr_), peer_rkey(peer_rkey_)
    {}
};

}

#endif // _ALLOCATION_RECORD_H_
