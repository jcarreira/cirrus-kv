/* Copyright 2016 Joao Carreira */

#include "src/server/ConnectionContext.h"
#include "src/utils/utils.h"

namespace sirius {

uint64_t ConnectionContext::id_count_ = 0;

ConnectionContext::ConnectionContext() :
    recv_msg(nullptr),
    recv_msg_mr(nullptr),
    send_msg(nullptr),
    send_msg_mr(nullptr),
    server(nullptr),
    info(0),
    context_id_(id_count_++) {
}

}  // namespace sirius
