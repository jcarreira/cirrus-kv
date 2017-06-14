#ifndef _FILE_ALLOCATION_RECORD_H_
#define _FILE_ALLOCATION_RECORD_H_

namespace cirrus {

/**
  * A struct that contains information on where to find an allocated file.
  */
struct FileAllocationRecord {
    /**
      * @brief The remote address where this file is stored.
      */
    uint64_t remote_addr; /** The remote address where this file is stored. */
    /**
      * @brief The peer_rkey for this file.
      */
    uint64_t peer_rkey;

    FileAllocationRecord(uint64_t remote_addr_, uint64_t peer_rkey_) :
        remote_addr(remote_addr_), peer_rkey(peer_rkey_)
    {}
};

}

#endif // _FILE_ALLOCATION_RECORD_H_
