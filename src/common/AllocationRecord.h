/* Copyright 2016 Joao Carreira */

#ifndef _ALLOCATION_RECORD_H_
#define _ALLOCATION_RECORD_H_

namespace sirius {

struct AllocationRecord {
    int alloc_id;
    uint64_t remote_addr;
    uint64_t peer_rkey;

    AllocationRecord(
            int alloc_id_ = 0, uint64_t remote_addr_ = 0, uint64_t peer_rkey_ = 0) :
        alloc_id(alloc_id_), remote_addr(remote_addr_), peer_rkey(peer_rkey_)
    {}
};

}

#endif // _ALLOCATION_RECORD_H_
