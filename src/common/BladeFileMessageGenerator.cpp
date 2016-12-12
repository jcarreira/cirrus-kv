/* Copyright 2016 Joao Carreira */

#include "src/common/BladeFileMessageGenerator.h"
#include <cstring>

namespace sirius {

void BladeFileMessageGenerator::alloc_msg(
        void *data, const std::string& filename, uint64_t size) {
    BladeFileMessage* msg =
        reinterpret_cast<BladeFileMessage*>(data);

    msg->type = ALLOC;
    std::memcpy(msg->data.alloc.filename, filename.c_str(), filename.size());
    msg->data.alloc.filename[filename.size()] = '\0';
    msg->data.alloc.size = size;
}

void BladeFileMessageGenerator::alloc_ack_msg(void *data,
        uint64_t remote_addr,
        uint64_t rkey) {
    BladeFileMessage* msg =
        reinterpret_cast<BladeFileMessage*>(data);

    msg->type = ALLOC_ACK;
    msg->data.alloc_ack.remote_addr = remote_addr;
    msg->data.alloc_ack.peer_rkey = rkey;
}

void BladeFileMessageGenerator::stats_msg(void *data) {
    BladeFileMessage* msg =
        reinterpret_cast<BladeFileMessage*>(data);

    msg->type = STATS;
}

void BladeFileMessageGenerator::stats_ack_msg(void *data) {
    BladeFileMessage* msg =
        reinterpret_cast<BladeFileMessage*>(data);

    msg->type = STATS_ACK;
}

}  // namespace sirius
