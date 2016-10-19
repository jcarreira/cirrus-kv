/* Copyright 2016 Joao Carreira */

#include "AllocatorMessageGenerator.h"

#include <cstring>

namespace sirius {

void AllocatorMessageGenerator::auth1_msg(void *data, AppId app_id) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = AUTH1;
    msg->data.auth1.app_id = app_id;
}

void AllocatorMessageGenerator::auth_ack1_msg(void *data, 
        AuthenticationToken auth_token) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(data);

    msg->type = AUTH_ACK1;
    msg->data.auth_ack1.challenge = 34; // random number from die draw
}

} // sirius
