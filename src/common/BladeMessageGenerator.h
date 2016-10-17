/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_MESSAGE_GENERATOR_H_
#define _BLADE_MESSAGE_GENERATOR_H_

#include "src/common/BladeMessage.h"
#include <stdint.h>

namespace sirius {

class BladeMessageGenerator {
public:
    static void alloc_msg(void *data, uint64_t size);
    static void alloc_ack_msg(void *data, 
            uint64_t mr_id,
            uint64_t remote_addr,
            uint64_t rkey);
};

void BladeMessageGenerator::alloc_msg(void *data, uint64_t size) {
    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(data);

    msg->type = ALLOC;
    msg->data.alloc.size = size;
}

void BladeMessageGenerator::alloc_ack_msg(void *data,
        uint64_t mr_id,
        uint64_t remote_addr,
        uint64_t rkey) {
    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(data);

    msg->type = ALLOC_ACK;
    msg->data.alloc_ack.mr_id = mr_id;
    msg->data.alloc_ack.remote_addr = remote_addr;
    msg->data.alloc_ack.peer_rkey = rkey;
}

} // sirius

#endif // _BLADE_MESSAGE_GENERATOR_H_

