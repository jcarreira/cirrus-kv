#include "server/ConnectionContext.h"
#include "utils/utils.h"

namespace cirrus {

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

}  // namespace cirrus
