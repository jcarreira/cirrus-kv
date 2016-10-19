/* Copyright 2016 Joao Carreira */

#ifndef _ALLOCATORMESSAGE_H_
#define _ALLOCATORMESSAGE_H_

#include "src/authentication/AuthenticationToken.h"

#include <cstdint>

namespace sirius {

enum auth_msg_type { AUTH1, AUTH_ACK1, AUTH2, AUTH_ACK2 };

typedef uint64_t AppId;

struct AllocatorMessage {
    int id;
    auth_msg_type type;
    union {
        // applications can ask
        // to be authenticated and get
        // a granting key
        struct {
            AppId app_id;
        } auth1;
        struct {
            uint64_t challenge;
        } auth_ack1;
        struct {
            uint64_t challenge_response;
        } auth2;
        struct {
            char allow;
            AuthenticationToken auth_token;
        } auth_ack2;
    } data;
};

} // sirius

#endif // _ALLOCATORMESSAGE_H_
