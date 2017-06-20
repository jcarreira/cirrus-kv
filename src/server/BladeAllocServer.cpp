#include <unistd.h>
#include <errno.h>
#include <boost/interprocess/creation_tags.hpp>
#include "server/BladeAllocServer.h"
#include "utils/logging.h"
#include "utils/Time.h"
#include "utils/InfinibandSupport.h"
#include "common/schemas/BladeMessage_generated.h"

namespace cirrus {

static const int SIZE = 1000000;
static const int initial_buffer_size = 50;

BladeAllocServer::BladeAllocServer(int port,
        uint64_t pool_size,
        int timeout_ms) :
    RDMAServer(port, timeout_ms),
    big_pool_mr_(0),
    big_pool_data_(0),
    big_pool_size_(pool_size),
    num_allocs_(0),
    mem_allocated(0) {
}

void BladeAllocServer::init() {
    // let upper layer initialize
    // all network related things
    RDMAServer::init();
}

void BladeAllocServer::handle_connection(struct rdma_cm_id* /*id*/) {
}

void BladeAllocServer::handle_disconnection(struct rdma_cm_id* /*id*/) {
}

/**
  * Allocates the initial memory pool.
  * @param size the size of the memory pool being created.
  * @return true
  */
bool BladeAllocServer::create_pool(uint64_t size) {
    TimerFunction tf("create_pool");

    LOG<INFO>("Allocating memory pool of size:", size);

    TEST_NZ(posix_memalign(reinterpret_cast<void**>(&big_pool_data_),
                static_cast<size_t>(sysconf(_SC_PAGESIZE)), size));

    LOG<INFO>("Creating memory region");
    TEST_Z(big_pool_mr_ = ibv_reg_mr(
                gen_ctx_.pd, big_pool_data_, size,
                IBV_ACCESS_LOCAL_WRITE |
                IBV_ACCESS_REMOTE_WRITE |
                IBV_ACCESS_REMOTE_READ));

    LOG<INFO>("Memory region created");

    // create allocator
    allocator.reset(new
        boost::interprocess::managed_external_buffer(
                boost::interprocess::create_only_t(), big_pool_data_, size));

    return true;
}

/**
  * Processes incoming RDMA messages.
  * Depending on the type of the message, responds differently. This function
  * handles allocation and deallocation.
  * @param id the rdma_cm_id used for communication
  * @param message a pointer to the buffer containing the new message.
  * @see allocate()
  * @see deallocate()
  */
void BladeAllocServer::process_message(rdma_cm_id* id,
        void* message) {
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    LOG<INFO>("Received message");

    // create a big poll
    // and allocate data from here
    std::call_once(pool_flag_,
            &BladeAllocServer::create_pool, this, big_pool_size_);

    // Read in the new message
    auto msg = message::BladeMessage::GetBladeMessage(message);

    // Instantiate the builder
    flatbuffers::FlatBufferBuilder builder(initial_buffer_size);

    // Check message type
    switch (msg->data_type()) {
        case message::BladeMessage::Data_Alloc:
            {
                uint64_t size = msg->data_as_Alloc()->size();

                LOG<INFO>("Received allocation request. size: ", size);
                void* ptr;
                uint64_t alloc_id;
                try {
                      ptr = allocator->allocate(size);
                      alloc_id = num_allocs_++;
                      id_to_alloc_[alloc_id] = BladeAllocation(ptr);
                      id_to_alloc_[alloc_id] = BladeAllocation(ptr);
                      mrs_data_.insert(ptr);
                } catch (const boost::interprocess::bad_alloc& e) {
                      /* Catch any alloc requests which failed. */
                      ptr = nullptr;
                      alloc_id = 0;
                }

                uint64_t remote_addr =
                    reinterpret_cast<uint64_t>(ptr);

                auto data = message::BladeMessage::CreateAllocAck(builder,
                                           alloc_id,
                                           remote_addr,
                                           big_pool_mr_->rkey);

                auto alloc_ack_msg =
                    message::BladeMessage::CreateBladeMessage(builder,
                                          message::BladeMessage::Data_AllocAck,
                                          data.Union());

                builder.Finish(alloc_ack_msg);

                int message_size = builder.GetSize();
                // Copy message into send buffer
                std::memcpy(ctx->send_msg,
                            builder.GetBufferPointer(),
                            message_size);

                LOG<INFO>("Sending ack. ",
                    " remote_addr:", remote_addr);

                // send async message
                send_message(id, message_size);

                break;
            }
        case message::BladeMessage::Data_Dealloc:
            {
                uint64_t addr = msg->data_as_Dealloc()->addr();

                allocator->deallocate(reinterpret_cast<void*>(addr));

                auto data = message::BladeMessage::CreateDeallocAck(
                                                                builder, true);
                auto dealloc_ack_msg =
                            message::BladeMessage::CreateBladeMessage(builder,
                                        message::BladeMessage::Data_DeallocAck,
                                        data.Union());

                builder.Finish(dealloc_ack_msg);
                int message_size = builder.GetSize();
                // Copy message into send buffer
                std::memcpy(ctx->send_msg,
                            builder.GetBufferPointer(),
                            message_size);

                LOG<INFO>("Deallocated addr: ", addr);

                // send async message
                send_message(id, message_size);

                break;
            }
        default:
            LOG<ERROR>("Unknown message", " type:", msg->data_type());
            exit(-1);
            break;
    }
}

}  // namespace cirrus
