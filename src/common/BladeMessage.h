/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_MESSAGE_H_
#define _BLADE_MESSAGE_H_

#include <cstdint>

namespace sirius {

enum msg_type { 
    ALLOC, ALLOC_ACK,
    STATS, STATS_ACK,
};

struct BladeMessage {
    msg_type type;
    union {
        struct {
            uint64_t size;
        } alloc;
        struct {
            uint64_t mr_id;
            uint64_t remote_addr;
            uint64_t peer_rkey;
        } alloc_ack;
    } data;
};

} // sirius

#endif // _BLADE_MESSAGE_H_

