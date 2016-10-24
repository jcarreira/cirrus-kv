/* Copyright 2016 Joao Carreira */

#include "src/common/AllocatorMessageGenerator.h"

#include <cstring>

namespace sirius {

void AllocatorMessageGenerator::auth1(void *data, AppId app_id) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = AUTH1;
    msg->data.auth1.app_id = app_id;
}

void AllocatorMessageGenerator::auth_ack1(void *data,
        char allow,
        uint64_t challenge) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = AUTH_ACK1;
    msg->data.auth_ack1.allow = allow;
    msg->data.auth_ack1.challenge = challenge;
}

void AllocatorMessageGenerator::auth2(void *data, uint64_t response) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = AUTH2;
    msg->data.auth2.response = response;
}

void AllocatorMessageGenerator::auth_ack2(void *data,
        char allow,
        AuthenticationToken auth_token) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = AUTH_ACK2;
    msg->data.auth_ack2.allow = allow;
    msg->data.auth_ack2.auth_token = auth_token;
}

void AllocatorMessageGenerator::stats(void *data) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = STATS;
}

void AllocatorMessageGenerator::stats_ack(void *data,
        uint64_t total_mem_allocated) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = STATS_ACK;
    msg->data.stats_ack.total_mem_allocated = total_mem_allocated;
}

void AllocatorMessageGenerator::alloc(void *data) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = ALLOC;
}

void AllocatorMessageGenerator::alloc_ack(void *data) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = ALLOC_ACK;
}

}  // namespace sirius
