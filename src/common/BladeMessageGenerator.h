/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_MESSAGE_GENERATOR_H_
#define _BLADE_MESSAGE_GENERATOR_H_

#include "src/common/BladeMessage.h"
#include <stdint.h>

namespace cirrus {

struct BladeMessageGenerator {
    // allocation
    static void alloc_msg(void *data, uint64_t size);
    static void alloc_ack_msg(void *data, 
            uint64_t mr_id,
            uint64_t remote_addr,
            uint64_t rkey);
    
    // deallocation
    static void dealloc_msg(void *data, uint64_t addr);
    static void dealloc_ack_msg(void *data, char result);

    // get stats
    static void stats_msg(void *data);
    static void stats_ack_msg(void *data);
};

} // cirrus

#endif // _BLADE_MESSAGE_GENERATOR_H_

