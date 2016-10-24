/* Copyright 2016 Joao Carreira */

#ifndef _ALLOC_MESSAGE_GENERATOR_H_
#define _ALLOC_MESSAGE_GENERATOR_H_

#include "src/common/AllocatorMessage.h"
#include "src/authentication/ApplicationKey.h"
#include <stdint.h>

namespace sirius {

class AllocatorMessageGenerator {
public:
    static void auth1(void *data, AppId app_id);
    static void auth_ack1(void *data, char allow, uint64_t challenge);
    static void auth2(void *data, uint64_t challenge);
    static void auth_ack2(void *data, char allow,
            AuthenticationToken auth_token);
    
    static void stats(void *data);
    static void stats_ack(void *data, uint64_t total_mem_allocated);
    
    static void alloc(void *data);
    static void alloc_ack(void *data);
};

} // sirius

#endif // _ALLOC_MESSAGE_GENERATOR_H_

