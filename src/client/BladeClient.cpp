/* Copyright 2016 Joao Carreira */

#include "src/client/BladeClient.h"
#include <unistd.h>
#include <string>
#include <cstring>
#include "src/common/BladeMessageGenerator.h"
#include "src/common/BladeMessage.h"
#include "src/utils/utils.h"
#include "src/utils/Time.h"
#include "src/utils/logging.h"
#include "src/client/AuthenticationClient.h"

namespace cirrus {

BladeClient::BladeClient(int timeout_ms)
    : RDMAClient(timeout_ms), remote_addr_(0) {
}

bool BladeClient::authenticate(std::string address,
        std::string port, AuthenticationToken& auth_token) {
    LOG<INFO>("BladeClient authenticating");
    AuthenticationClient auth_client;

    LOG<INFO>("BladeClient connecting to controller");
    auth_client.connect(address, port);

    LOG<INFO>("BladeClient authenticating");
    auth_token = auth_client.authenticate();

    return auth_token.allow;
}

AllocationRecord BladeClient::allocate(uint64_t size) {
    LOG<INFO>("Allocating ",
        size, " bytes");

    flatbuffers::FlatBufferBuilder builder(48);
    auto data = CreateAlloc(builder, size);
    auto alloc_msg = CreateBladeMessage(builder, Data_Alloc, data.Union());
    builder.Finish(alloc_msg);

    int message_size = builder.GetSize();
    //copy message over
    std::memcpy(con_ctx_.send_msg,
                builder.GetBufferPointer(),
                message_size);


    // post receive
    LOG<INFO>("Sending alloc msg size: ", sizeof(BladeMessage));
    send_receive_message_sync(id_, sizeof(BladeMessage));
    LOG<INFO>("send_receive_message_sync done: ", sizeof(BladeMessage));

    auto msg = GetBladeMessage(con_ctx_.recv_msg);

    AllocationRecord alloc(
                msg->data_as_AllocAck()->mr_id(),
                msg->data_as_AllocAck()->remote_addr(),
                msg->data_as_AllocAck()->peer_rkey());

    LOG<INFO>("Received allocation. mr_id: ", msg->data_as_AllocAck()->mr_id(),
            " remote_addr: " , msg->data_as_AllocAck()->remote_addr(),
            " peer_rkey: ", msg->data_as_AllocAck()->peer_rkey());

    if (msg->data_as_AllocAck()->remote_addr() == 0)
        throw std::runtime_error("Error with allocation");

    LOG<INFO>("Received allocation from Blade. remote_addr: ",
        msg->data_as_AllocAck()->remote_addr(),
        " mr_id: ", msg->data_as_AllocAck()->mr_id());
    return alloc;
}

bool BladeClient::deallocate(const AllocationRecord& ar) {
    LOG<INFO>("Deallocating addr: ", ar.remote_addr);

    flatbuffers::FlatBufferBuilder builder(48);
    auto data = CreateDealloc(builder, ar.remote_addr);
    auto dealloc_msg = CreateBladeMessage(builder, Data_Dealloc, data.Union());
    builder.Finish(dealloc_msg);

    int message_size = builder.GetSize();
    //copy message over
    std::memcpy(con_ctx_.send_msg,
                builder.GetBufferPointer(),
                message_size);


    // post receive
    LOG<INFO>("Sending dealloc msg size: ", sizeof(BladeMessage));
    send_receive_message_sync(id_, message_size);
    LOG<INFO>("send_receive_message_sync done: ", sizeof(BladeMessage));

    auto msg = GetBladeMessage(con_ctx_.recv_msg);

    if (msg->data_as_DeallocAck()->result() == 0)
        throw std::runtime_error("Error with deallocation");
    return true;
}

bool BladeClient::write_sync(const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data,
        RDMAMem* mem) {
    LOG<INFO>("writing rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    if (length > SEND_MSG_SIZE)
        return false;

    if (mem) {
        {
            // TimerFunction tf("BladeClient::write_sync prepare", true);
            mem->addr_ = reinterpret_cast<uint64_t>(data);
            mem->prepare(con_ctx_.gen_ctx_);
        }

        write_rdma_sync(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey,
                *mem);

    } else {
        std::memcpy(con_ctx_.send_msg, data, length);
        write_rdma_sync(id_, length,
                alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
                default_send_mem_);
    }

    return true;
}

std::shared_ptr<FutureBladeOp> BladeClient::write_async(
        const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data,
        RDMAMem* mem) {
    LOG<INFO>("writing rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    if (length > SEND_MSG_SIZE)
        return nullptr;

    RDMAOpInfo* op_info = nullptr;
    if (mem) {
        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->size_ = length;
        mem->prepare(con_ctx_.gen_ctx_);

        op_info = write_rdma_async(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey,
                *mem);
    } else {
        // TimerFunction tf("write_async memcpy", true);
        std::memcpy(con_ctx_.send_msg, data, length);
        op_info = write_rdma_async(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey,
                default_send_mem_);
    }

    return std::make_shared<FutureBladeOp>(op_info);
}

bool BladeClient::read_sync(const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t length,
        void *data,
        RDMAMem* mem) {
    if (length > RECV_MSG_SIZE)
        return false;

    LOG<INFO>("reading rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    if (mem) {
        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->size_ = length;
        mem->prepare(con_ctx_.gen_ctx_);

        read_rdma_sync(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey, *mem);

    } else {
        read_rdma_sync(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey, default_recv_mem_);

        {
            TimerFunction tf("Memcpy time", true);
            std::memcpy(data, con_ctx_.recv_msg, length);
        }
    }

    return true;
}

std::shared_ptr<FutureBladeOp> BladeClient::read_async(
        const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t length,
        void *data,
        RDMAMem* mem) {
    if (length > RECV_MSG_SIZE)
        return nullptr;

    LOG<INFO>("reading rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    RDMAOpInfo* op_info;
    if (mem) {
        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->prepare(con_ctx_.gen_ctx_);

        op_info = read_rdma_async(id_, length,
                alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
                *mem,
                []() -> void {});
    } else {
        void* buffer = con_ctx_.recv_msg;  // need this for capture list
        auto copy_fn = [data, buffer, length]() {
            TimerFunction tf("Memcpy (read) time", true);
            std::memcpy(data, buffer, length);
        };
        op_info = read_rdma_async(id_, length,
                alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
                default_recv_mem_, copy_fn);
    }

    return std::make_shared<FutureBladeOp>(op_info);
}

bool BladeClient::fetchadd_sync(const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t value) {
    LOG<INFO>("fetchadd (sync) rdma",
            " offset: ", offset,
            " value: ", value);

    fetchadd_rdma_sync(id_,
            alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
            value);

    return true;
}

std::shared_ptr<FutureBladeOp> BladeClient::fetchadd_async(
        const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t value) {
    LOG<INFO>("fetchadd (sync) rdma",
            " offset: ", offset,
            " value: ", value);

    RDMAOpInfo* op_info = fetchadd_rdma_async(id_,
            alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
            value);

    return std::make_shared<FutureBladeOp>(op_info);
}

void FutureBladeOp::wait() {
    op_info->op_sem->wait();
}

bool FutureBladeOp::try_wait() {
    return op_info->op_sem->trywait();
}

}  // namespace cirrus
