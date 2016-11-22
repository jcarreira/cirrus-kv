/* Copyright 2016 Joao Carreira */

#include "src/client/AuthenticationClient.h"
#include "src/common/AllocatorMessage.h"
#include "src/common/AllocatorMessageGenerator.h"
#include "src/utils/utils.h"
#include "src/utils/logging.h"

namespace sirius {

AuthenticationClient::AuthenticationClient(int timeout_ms) :
    RDMAClient(timeout_ms) {
}

AuthenticationToken AuthenticationClient::authenticate() {
    LOG<INFO>("AuthenticationClient authenticating");

    AppId app_id = 1;
    AllocatorMessageGenerator::auth1(con_ctx.send_msg, app_id);

    // post receive
    LOG<INFO>("Sending auth1 msg size: ", sizeof(AllocatorMessage));
    send_receive_message_sync(id_, sizeof(AllocatorMessage));

    auto msg = reinterpret_cast<AllocatorMessage*>(con_ctx.recv_msg);
    LOG<INFO>("Received challenge: ", msg->data.auth_ack1.challenge);

    return AuthenticationToken(true);
}

}  // namespace sirius
