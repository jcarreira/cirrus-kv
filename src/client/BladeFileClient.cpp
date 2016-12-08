/* Copyright 2016 Joao Carreira */

#include "src/client/BladeFileClient.h"
#include <unistd.h>
#include <string>
#include <cstring>
#include "src/common/BladeFileMessageGenerator.h"
#include "src/common/BladeFileMessage.h"
#include "src/utils/utils.h"
#include "src/utils/TimerFunction.h"
#include "src/client/AuthenticationClient.h"
#include "src/utils/logging.h"

namespace sirius {

BladeFileClient::BladeFileClient(int timeout_ms)
    : RDMAClient(timeout_ms), remote_addr_(0) {
}

BladeFileClient::~BladeFileClient() {
}

bool BladeFileClient::authenticate(std::string address,
        std::string port, AuthenticationToken& auth_token) {
    LOG<INFO>("BladeFileClient authenticating");
    AuthenticationClient auth_client;

    LOG<INFO>("BladeFileClient connecting to controller");
    auth_client.connect(address, port);

    LOG<INFO>("BladeFileClient authenticating");
    auth_token = auth_client.authenticate();

    return auth_token.allow;
}

FileAllocRec BladeFileClient::allocate(const std::string& filename,
        uint64_t size) {
    LOG<INFO>("Allocating ", size, " bytes");

    BladeFileMessageGenerator::alloc_msg(con_ctx.send_msg,
            filename,
            size);

    LOG<INFO>("Sending alloc msg size: ", sizeof(BladeFileMessage));
    send_receive_message_sync(id_, sizeof(BladeFileMessage));

    BladeFileMessage* msg =
        reinterpret_cast<BladeFileMessage*>(con_ctx.recv_msg);

    FileAllocRec alloc(new FileAllocationRecord(
                msg->data.alloc_ack.remote_addr,
                msg->data.alloc_ack.peer_rkey));

    LOG<INFO>("Received allocation from Blade. remote_addr: ",
        msg->data.alloc_ack.remote_addr);
    return alloc;
}

bool BladeFileClient::write_sync(const FileAllocRec& alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data) {
    LOG<INFO>("writing rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec->remote_addr,
        " rkey: ", alloc_rec->peer_rkey);

    if (length > SEND_MSG_SIZE)
        return false;

    // FIX: build a wrapper around memcpy
    // to deal with memory size limitations
    std::memcpy(con_ctx.send_msg, data, length);
    write_rdma_sync(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey,
            default_send_mem);

    return true;
}

bool BladeFileClient::write(const FileAllocRec& alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data) {
    LOG<INFO>("writing rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec->remote_addr,
        " rkey: ", alloc_rec->peer_rkey);

    if (length > SEND_MSG_SIZE)
        return false;

    std::memcpy(con_ctx.send_msg, data, length);
    write_rdma_sync(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey,
            default_send_mem);

    return true;
}

bool BladeFileClient::read_sync(const FileAllocRec& alloc_rec,
        uint64_t offset,
        uint64_t length,
        void *data) {
    if (length > RECV_MSG_SIZE)
        return false;

    LOG<INFO>("reading rdma"
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec->remote_addr,
        " rkey: ", alloc_rec->peer_rkey);

    read_rdma_sync(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey,
            default_recv_mem);

    {
        TimerFunction tf("Memcpy time", true);
        std::memcpy(data, con_ctx.recv_msg, length);
    }

    return true;
}

bool BladeFileClient::read(const FileAllocRec& alloc_rec,
        uint64_t offset,
        uint64_t length,
        void *data) {
    if (length > RECV_MSG_SIZE)
        return false;

    LOG<INFO>("reading rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec->remote_addr,
        " rkey: ", alloc_rec->peer_rkey);

    read_rdma_sync(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey,
            default_recv_mem);

    {
        TimerFunction tf("Memcpy time", true);
        std::memcpy(data, con_ctx.recv_msg, length);
    }

    return true;
}

}  // namespace sirius
