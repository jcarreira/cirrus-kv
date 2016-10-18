/* Copyright 2016 Joao Carreira */

#include "src/server/ConnectionContext.h"
#include "src/utils/utils.h"

namespace sirius {
    
uint64_t ConnectionContext::id_count_ = 0;

ConnectionContext::ConnectionContext() :
    recv_msg(NULL),
    recv_msg_mr(NULL),
    send_msg(NULL),
    send_msg_mr(NULL),
    server(NULL),
    context_id_(id_count_++) {
}

ConnectionContext::~ConnectionContext() {

}

}
