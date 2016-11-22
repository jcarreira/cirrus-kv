/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_FILE_MESSAGE_GENERATOR_H_
#define _BLADE_FILE_MESSAGE_GENERATOR_H_

#include "src/common/BladeFileMessage.h"
#include <stdint.h>
#include <string>

namespace sirius {

struct BladeFileMessageGenerator {
    static void alloc_msg(void *data, 
            const std::string& filename, uint64_t size);
    static void alloc_ack_msg(void *data, 
            uint64_t remote_addr,
            uint64_t rkey);
    static void stats_msg(void *data);
    static void stats_ack_msg(void *data);
};

} // sirius

#endif // _BLADE_FILE_MESSAGE_GENERATOR_H_

