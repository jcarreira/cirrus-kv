/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_OBJECT_STORE_MESSAGE_GENERATOR_H_
#define _BLADE_OBJECT_STORE_MESSAGE_GENERATOR_H_


#include "src/common/BladeMessage.h"
#include "src/common/BladeMessageGenerator.h"
#include <stdint.h>

namespace cirrus {

struct BladeObjectStoreMessageGenerator : public BladeMessageGenerator {
    // keep alive
    static void keep_alive_msg(void *data, uint64_t rand);
    static void keep_alive_ack_msg(void *data, uint64_t rand);
    
    // flush
    static void flush_msg(void *data, uint64_t rand);
    static void flush_ack_msg(void *data, uint64_t rand);
    
    // lock
    static void lock_msg(void *data, uint64_t rand);
    static void lock_ack_msg(void *data, uint64_t rand);

    // pub subscribe
    static void subscribe_msg(void *data, uint64_t);
    static void subscribe_ack_msg(void *data, uint64_t);

};

} // cirrus

#endif // _BLADE_OBJECT_STORE_MESSAGE_GENERATOR_H_

