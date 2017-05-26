/* Copyright 2016 Joao Carreira */

#include "src/client/BladeFileClient.h"
#include <unistd.h>
#include <string>
#include <cstring>
#include "src/utils/utils.h"
#include "src/utils/Time.h"
#include "src/client/AuthenticationClient.h"
#include "src/utils/logging.h"
#include "src/common/schemas/BladeFileMessage_generated.h"
using namespace cirrus::Message::BladeFileMessage;


namespace cirrus {

BladeFileClient::BladeFileClient(int timeout_ms)
    : RDMAClient(timeout_ms), remote_addr_(0) {
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

    flatbuffers::FlatBufferBuilder builder(50);

    auto serialized_filename = builder.CreateString(filename);
    auto data = CreateAlloc(builder, size, serialized_filename);

    auto alloc_msg = CreateBladeFileMessage(builder, Data_Alloc, data.Union());

    builder.Finish(alloc_msg);

    int message_size = builder.GetSize();
    LOG<INFO>("Sending alloc msg size: ", message_size);

    //copy message over
    std::memcpy(con_ctx_.send_msg,
                builder.GetBufferPointer(),
                message_size);

    send_receive_message_sync(id_, message_size);

    //receive ack flatbuffer in response
    auto msg = GetBladeFileMessage(con_ctx_.recv_msg);

    FileAllocRec alloc(
                msg->data_as_AllocAck()->remote_addr(),
                msg->data_as_AllocAck()->peer_rkey());

    LOG<INFO>("Received allocation from Blade. remote_addr: ",
        msg->data_as_AllocAck()->remote_addr());
    return alloc;
}

bool BladeFileClient::write_sync(const FileAllocRec& alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data) {
    LOG<INFO>("writing rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    if (length > SEND_MSG_SIZE)
        return false;

    RDMAMem mem(data, length);
    mem.prepare(con_ctx_.gen_ctx_);

    write_rdma_sync(id_, length,
            alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
            mem);
    mem.clear();

    return true;
}

std::shared_ptr<FutureBladeOp> BladeFileClient::write_async(
        const FileAllocRec& alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data,
        RDMAMem& mem) {
    LOG<INFO>("writing rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    if (length > SEND_MSG_SIZE)
        return nullptr;

    mem.addr_ = reinterpret_cast<uint64_t>(data);
    mem.size_ = length;
    mem.mr = 0;

    TEST_NZ(mem.prepare(con_ctx_.gen_ctx_));

    RDMAOpInfo* op_info = write_rdma_async(id_, length,
            alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
            mem);

    // client does mem.clear() or let object be destroyed

    return std::make_shared<FutureBladeOp>(op_info);
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
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    RDMAMem mem(data, length);
    mem.prepare(con_ctx_.gen_ctx_);
    read_rdma_sync(id_, length,
            alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
            mem);
    mem.clear();

    return true;
}

std::shared_ptr<FutureBladeOp> BladeFileClient::read_async(
        const FileAllocRec& alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void *data,
        RDMAMem& mem) {
    if (length > RECV_MSG_SIZE)
        return nullptr;

    LOG<INFO>("reading (async) rdma"
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    mem.addr_ = reinterpret_cast<uint64_t>(data);
    mem.size_ = length;
    mem.mr = 0;
    mem.prepare(con_ctx_.gen_ctx_);

    mem.prepare(con_ctx_.gen_ctx_);
    RDMAOpInfo* op_info = read_rdma_async(id_, length,
            alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
            mem);

    // client of this function needs to call mem.clear()
    // or let RDMAMem be destroyed

    return std::make_shared<FutureBladeOp>(op_info);
}

}  // namespace cirrus
