/* Copyright 2016 Joao Carreira */

#include "src/common/BladeMessageGenerator.h"

namespace sirius {

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

void BladeMessageGenerator::dealloc_msg(void *data, uint64_t addr) {
    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(data);

    msg->type = DEALLOC;
    msg->data.dealloc.addr = addr;
}

void BladeMessageGenerator::dealloc_ack_msg(void *data,
        char result) {
    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(data);

    msg->type = DEALLOC_ACK;
    msg->data.dealloc_ack.result = result;
}

void BladeMessageGenerator::stats_msg(void *data) {
    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(data);

    msg->type = STATS;
}

void BladeMessageGenerator::stats_ack_msg(void *data) {
    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(data);

    msg->type = STATS_ACK;
}

}  // namespace sirius
