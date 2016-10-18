/* Copyright 2016 Joao Carreira */

#ifndef _ALLOCATORMESSAGE_H_
#define _ALLOCATORMESSAGE_H_

#include "src/authentication/ApplicationKey.h"

namespace sirius {

enum auth_msg_type { AUTH };

typedef uint64_t AppId;

struct AllocatorMessage {
    int id;
    auth_msg_type type;
    union {
        struct {
            AppId app_id;
            ApplicationKey application_key;
        } auth;
    } data;
};

} // sirius

#endif // _ALLOCATORMESSAGE_H_
