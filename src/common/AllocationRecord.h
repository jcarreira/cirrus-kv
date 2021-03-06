#ifndef SRC_COMMON_ALLOCATIONRECORD_H_
#define SRC_COMMON_ALLOCATIONRECORD_H_

namespace cirrus {

/**
  * Contains information tracking an allocation.
  */
struct AllocationRecord {
    int alloc_id;
    uint64_t remote_addr;
    uint64_t peer_rkey;

    AllocationRecord(
            int alloc_id_ = 0,
            uint64_t remote_addr_ = 0,
            uint64_t peer_rkey_ = 0) :
        alloc_id(alloc_id_), remote_addr(remote_addr_), peer_rkey(peer_rkey_)
    {}
};

}  // namespace cirrus

#endif  // SRC_COMMON_ALLOCATIONRECORD_H_
