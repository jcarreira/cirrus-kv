/* Copyright 2016 Joao Carreira */

#include "src/server/ConnectionContext.h"
#include "src/utils/utils.h"

namespace sirius {

ConnectionContext::ConnectionContext() :
    recv_msg(NULL),
    recv_msg_mr(NULL),
    send_msg(NULL),
    send_msg_mr(NULL),
    server(NULL),
    context_id(0) {
    TEST_NZ(sem_init(&ack_sem, 0, 0));
}

}
