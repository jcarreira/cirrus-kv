/* Copyright 2016 Joao Carreira */

#ifndef _ALLOC_MESSAGE_GENERATOR_H_
#define _ALLOC_MESSAGE_GENERATOR_H_

#include "src/common/AllocatorMessage.h"
#include "src/authentication/ApplicationKey.h"
#include <stdint.h>

namespace sirius {

class AllocatorMessageGenerator {
public:
    static void auth1_msg(void *data, AppId app_id);
    static void auth_ack1_msg(void *data, AuthenticationToken auth_token);
};

} // sirius

#endif // _ALLOC_MESSAGE_GENERATOR_H_

