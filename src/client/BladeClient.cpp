/* Copyright 2016 Joao Carreira */

#include "src/client/BladeClient.h"
#include <unistd.h>
#include <string>
#include "src/common/BladeMessageGenerator.h"
#include "src/common/BladeMessage.h"
#include "src/utils/utils.h"
#include "src/utils/TimerFunction.h"
#include "third_party/easylogging++.h"
#include "src/client/AuthenticationClient.h"

namespace sirius {

BladeClient::BladeClient(int timeout_ms)
    : RDMAClient(timeout_ms) {
}

BladeClient::~BladeClient() {
}

bool BladeClient::authenticate(std::string address,
        std::string port, AuthenticationToken& auth_token) {
    LOG(INFO) << "BladeClient authenticating";
    AuthenticationClient auth_client;

    LOG(INFO) << "BladeClient connecting to controller";
    auth_client.connect(address, port);

    LOG(INFO) << "BladeClient authenticating";
    auth_token = auth_client.authenticate();

    return auth_token.allow;
}

AllocRec BladeClient::allocate(uint64_t size) {
    LOG(INFO) << "Allocating " << size << " bytes";

    BladeMessageGenerator::alloc_msg(con_ctx.send_msg,
            size);

    // post receive
    TEST_NZ(post_receive(id_));
    LOG(INFO) << "Sending alloc msg size: " << sizeof(BladeMessage);
    send_message_sync(id_, sizeof(BladeMessage));

    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(con_ctx.recv_msg);

    AllocRec alloc(new AllocationRecord(
              msg->data.alloc_ack.mr_id,
                msg->data.alloc_ack.remote_addr,
                msg->data.alloc_ack.peer_rkey));

    LOG(INFO) << "Received allocation from Blade. remote_addr: "
        << msg->data.alloc_ack.remote_addr
        << " mr_id: " << msg->data.alloc_ack.mr_id;
    return alloc;
}

bool BladeClient::write_sync(AllocRec alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data) {
    LOG(INFO) << "writing rdma"
        << " length: " << length
        << " offset: " << offset
        << " remote_addr: " << alloc_rec->remote_addr
        << " rkey: " << alloc_rec->peer_rkey;

    if (length > SEND_MSG_SIZE)
        return false;

    // FIX: build a wrapper around memcpy
    // to deal with memory size limitations
    std::memcpy(con_ctx.send_msg, data, length);
    write_rdma_sync(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey);

    return true;
}

bool BladeClient::write(AllocRec alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data) {
    LOG(INFO) << "writing rdma"
        << " length: " << length
        << " offset: " << offset
        << " remote_addr: " << alloc_rec->remote_addr
        << " rkey: " << alloc_rec->peer_rkey;

    if (length > SEND_MSG_SIZE)
        return false;

    std::memcpy(con_ctx.send_msg, data, length);
    write_rdma(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey);

    return true;
}

bool BladeClient::read_sync(AllocRec alloc_rec,
        uint64_t offset,
        uint64_t length,
        void *data) {
    if (length > RECV_MSG_SIZE)
        return false;

    LOG(INFO) << "reading rdma"
        << " length: " << length
        << " offset: " << offset
        << " remote_addr: " << alloc_rec->remote_addr
        << " rkey: " << alloc_rec->peer_rkey;

    read_rdma_sync(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey);

    {
        TimerFunction tf("Memcpy time", true);
        std::memcpy(data, con_ctx.recv_msg, length);
    }

    return true;
}

bool BladeClient::read(AllocRec alloc_rec,
        uint64_t offset,
        uint64_t length,
        void *data) {
    if (length > RECV_MSG_SIZE)
        return false;

    LOG(INFO) << "reading rdma"
        << " length: " << length
        << " offset: " << offset
        << " remote_addr: " << alloc_rec->remote_addr
        << " rkey: " << alloc_rec->peer_rkey;

    read_rdma(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey);

    {
        TimerFunction tf("Memcpy time", true);
        std::memcpy(data, con_ctx.recv_msg, length);
    }

    return true;
}

}  // namespace sirius
