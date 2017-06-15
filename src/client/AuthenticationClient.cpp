#include "client/AuthenticationClient.h"
#include "common/AllocatorMessage.h"
#include "common/AllocatorMessageGenerator.h"
#include "utils/utils.h"
#include "utils/logging.h"

namespace cirrus {

AuthenticationClient::AuthenticationClient(int timeout_ms) :
    RDMAClient(timeout_ms) {
}

AuthenticationToken AuthenticationClient::authenticate() {
    LOG<INFO>("AuthenticationClient authenticating");

    AppId app_id = 1;
    AllocatorMessageGenerator::auth1(con_ctx_.send_msg, app_id);

    // post receive
    LOG<INFO>("Sending auth1 msg size: ", sizeof(AllocatorMessage));
    send_receive_message_sync(id_, sizeof(AllocatorMessage));

    auto msg = reinterpret_cast<AllocatorMessage*>(con_ctx_.recv_msg);
    LOG<INFO>("Received challenge: ", msg->data.auth_ack1.challenge);

    return AuthenticationToken(true);
}

}  // namespace cirrus
