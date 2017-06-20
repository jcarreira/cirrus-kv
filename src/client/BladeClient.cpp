#include "client/BladeClient.h"
#include <unistd.h>
#include <string>
#include <cstring>
#include "utils/utils.h"
#include "utils/Time.h"
#include "utils/logging.h"
#include "client/AuthenticationClient.h"
#include "common/Exception.h"
#include "common/schemas/BladeMessage_generated.h"
namespace cirrus {

static const int initial_buffer_size = 50;

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

/**
  * A function that requests a given number of bytes be allocated
  * from the server.
  * @param size the number of bytes the client is requesting be allocated
  * @return AllocationRecord for the new AllocationRecord
  */
AllocationRecord BladeClient::allocate(uint64_t size) {
    LOG<INFO>("Allocating ",
        size, " bytes");

    // Create message using flatbuffers
    flatbuffers::FlatBufferBuilder builder(initial_buffer_size);
    auto data = message::BladeMessage::CreateAlloc(builder, size);
    auto alloc_msg = message::BladeMessage::CreateBladeMessage(builder,
                              message::BladeMessage::Data_Alloc, data.Union());
    builder.Finish(alloc_msg);

    int message_size = builder.GetSize();

    // Copy message contents into send buffer
    std::memcpy(con_ctx_.send_msg,
                builder.GetBufferPointer(),
                message_size);


    // post receive
    LOG<INFO>("Sending alloc msg size: ", message_size);
    send_receive_message_sync(id_, message_size);
    LOG<INFO>("send_receive_message_sync done: ", message_size);

    auto msg = message::BladeMessage::GetBladeMessage(con_ctx_.recv_msg);

    if (msg->data_as_AllocAck()->remote_addr() == 0) {
      // Throw error message
      LOG<ERROR>("Server threw exception when allocating memory.");
      throw cirrus::ServerMemoryErrorException("Server threw "
               "exception when allocating memory.");
    }

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

/**
  * A function that requests a given allocation be freed in the remote store.
  * @param ar the allocation record that records the allocation to be freed.
  * @return success
  */
bool BladeClient::deallocate(const AllocationRecord& ar) {
    LOG<INFO>("Deallocating addr: ", ar.remote_addr);

    flatbuffers::FlatBufferBuilder builder(initial_buffer_size);
    auto data = message::BladeMessage::CreateDealloc(builder, ar.remote_addr);
    auto dealloc_msg = message::BladeMessage::CreateBladeMessage(builder,
                            message::BladeMessage::Data_Dealloc, data.Union());
    builder.Finish(dealloc_msg);
    int message_size = builder.GetSize();
    // Copy message into send buffer
    std::memcpy(con_ctx_.send_msg,
                builder.GetBufferPointer(),
                message_size);


    // post receive
    LOG<INFO>("Sending dealloc msg size: ", message_size);
    send_receive_message_sync(id_, message_size);
    LOG<INFO>("send_receive_message_sync done: ", message_size);

    auto msg = message::BladeMessage::GetBladeMessage(con_ctx_.recv_msg);

    if (msg->data_as_DeallocAck()->result() == 0)
        throw std::runtime_error("Error with deallocation");
    return true;
}

/**
  * Write synchronously.
  * A function that writes data to the remote store synchronously. Reads from
  * data and passes length bytes.
  * @param alloc_rec the AllocationRecord corresponding to an adequately sized
  * allocation.
  * @param offset the offset from data at which to start reading
  * @param length the number of bytes to send
  * @param mem pointer to RDMAMem struct for this write
  * @return write success
  * @see AllocationRecord
  */
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

/**
  * Write asynchronously.
  * A function that writes data to the remote store asynchronously. Reads from
  * data and passes length bytes.
  * @param alloc_rec the AllocationRecord corresponding to an adequately sized
  * allocation.
  * @param offset the offset from data at which to start reading
  * @param length the number of bytes to send
  * @param mem pointer to RDMAMem struct for this write
  * @return std::shared_ptr<FutureBladeOp> a shared_ptr to a FutureBladeOp
  * that the caller can then extract a future from. Null if length >
  * the max sendable message size
  * @see AllocationRecord
  */
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
        RDMAMem* mem = new RDMAMem(data, length);

        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->prepare(con_ctx_.gen_ctx_);

        op_info = write_rdma_async(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey,
                *mem);
    }

    return std::make_shared<FutureBladeOp>(op_info);
}

/**
  * Read synchronously.
  * A function that reads data from the remote store synchronously. Reads
  * length bytes offset by offset bytes from data.
  * @param alloc_rec the AllocationRecord containing the necessary info
  * @param offset the offset from the remote_addr at which to start reading
  * @param length the number of bytes to read
  * @param data the location in local memory to write to
  * @param mem pointer to RDMAMem struct for this read
  * @return success if length is less than max receivable size
  */
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

/**
  * Read asynchronously.
  * A function that reads data from the remote store asynchronously. Reads
  * length bytes offset by offset bytes from the remote address.
  * @param alloc_rec the AllocationRecord containing the necessary info
  * @param offset the offset from the remote_addr at which to start reading
  * @param length the number of bytes to read
  * @param data the location in local memory to write to
  * @param mem pointer to RDMAMem struct for this read
  * @return std::shared_ptr<FutureBladeOp> a shared_ptr to a FutureBladeOp
  * that the caller can then extract a future from. Null if length >
  * the max receivable message size
  */
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
        RDMAMem* mem = new RDMAMem(data, length);

        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->prepare(con_ctx_.gen_ctx_);

        op_info = read_rdma_async(id_, length,
                alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
                *mem,
                [mem]() -> void { delete mem; });
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


/**
  * Wait for operation to finish.
  * A function that calls the FutureBladeOp's operation's wait() method.
  * Waits until the operation is complete.
  * @see RDMAOpInfo
  * @see Lock
  */
void FutureBladeOp::wait() {
    op_info->op_sem->wait();
}

/**
  * Check operation status.
  * A function that calls the FutureBladeOp's operation's try_wait() method.
  * Returns instantly with the status of the operation.
  * @return true if operation is finished, false otherwise
  * @see RDMAOpInfo
  * @see Lock
  */
bool FutureBladeOp::try_wait() {
    return op_info->op_sem->trywait();
}

}  // namespace cirrus
