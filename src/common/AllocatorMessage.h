/* Copyright 2016 Joao Carreira */

#ifndef _ALLOCATORMESSAGE_H_
#define _ALLOCATORMESSAGE_H_

#include "src/authentication/AuthenticationToken.h"

#include <cstdint>

namespace sirius {

enum auth_msg_type {
    AUTH1, AUTH_ACK1, AUTH2, AUTH_ACK2,
    STATS, STATS_ACK,
    ALLOC, ALLOC_ACK,
};

using AppId = uint64_t;

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
            char allow;
            uint64_t challenge;
        } auth_ack1;
        struct {
            uint64_t response;
        } auth2;
        struct {
            char allow;
            AuthenticationToken auth_token;
        } auth_ack2;
        struct {
        } stats;
        struct {
            uint64_t total_mem_allocated;
        } stats_ack;
        struct {
        } alloc;
        struct {
        } alloc_ack;
    } data;
};

} // sirius

#endif // _ALLOCATORMESSAGE_H_
