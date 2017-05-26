/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <errno.h>
#include <boost/interprocess/creation_tags.hpp>
#include "src/server/BladeObjectStore.h"
#include "src/utils/logging.h"
#include "src/utils/Time.h"
#include "src/utils/InfinibandSupport.h"
#include "src/common/schemas/BladeObjectStoreMessage_generated.h"
using namespace Message::BladeObjectStoreMessage;

namespace cirrus {

static const int SIZE = 1000000;

BladeObjectStore::BladeObjectStore(int port,
        uint64_t pool_size,
        int timeout_ms) :
    RDMAServer(port, timeout_ms),
    big_pool_mr_(0),
    big_pool_data_(0),
    big_pool_size_(pool_size),
    num_allocs_(0),
    mem_allocated(0) {
}

void BladeObjectStore::init() {
    // let upper layer initialize
    // all network related things
    RDMAServer::init();
}

void BladeObjectStore::handle_connection(struct rdma_cm_id* /*id*/) {
}

void BladeObjectStore::handle_disconnection(struct rdma_cm_id* /*id*/) {
}

bool BladeObjectStore::create_pool(uint64_t size) {
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

void BladeObjectStore::process_message(rdma_cm_id* id,
        void* message) {
    auto msg = GetBladeObjectStoreMessage(message);
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    LOG<INFO>("Received message");

    // create a big poll
    // and allocate data from here
    std::call_once(pool_flag_,
            &BladeObjectStore::create_pool, this, big_pool_size_);

    flatbuffers::FlatBufferBuilder builder(48);

    switch (msg->data_type()) {
        case Data_Alloc: {
                uint64_t size = msg->data_as_Alloc()->size();

                LOG<INFO>("Received allocation request. size: ", size);
                void* ptr = allocator->allocate(size);

                uint64_t remote_addr =
                    reinterpret_cast<uint64_t>(ptr);

                uint64_t alloc_id = num_allocs_++;
                id_to_alloc_[alloc_id] = BladeAllocation(ptr);

                mrs_data_.insert(ptr);

                auto data = CreateAllocAck(builder,
                                           alloc_id,
                                           remote_addr,
                                           big_pool_mr_->rkey);

                auto alloc_ack_msg = CreateBladeObjectStoreMessage(
                                                          builder,
                                                          Data_AllocAck,
                                                          data.Union());
                builder.Finish(alloc_ack_msg);

                int message_size = builder.GetSize();
                //copy message over
                std::memcpy(ctx->send_msg,
                            builder.GetBufferPointer(),
                            message_size);

                LOG<INFO>("Sending ack. ",
                    " remote_addr:", remote_addr);

                // send async message
                send_message(id, message_size);

                break;
            }
        case Data_Dealloc: {
                uint64_t addr = msg->data_as_Dealloc()->addr();

                allocator->deallocate(reinterpret_cast<void*>(addr));

                auto data = CreateDeallocAck(builder, true);
                auto dealloc_ack_msg = CreateBladeObjectStoreMessage(builder,
                                                          Data_DeallocAck,
                                                          data.Union());
                builder.Finish(dealloc_ack_msg);
                int message_size = builder.GetSize();
                //copy message over
                std::memcpy(ctx->send_msg,
                            builder.GetBufferPointer(),
                            message_size);

                LOG<INFO>("Deallocated addr: ", addr);

                // send async message
                send_message(id, message_size);

                break;
            }
        case Data_KeepAlive: {
                uint64_t rand = msg->data_as_KeepAlive()->rand();

                flatbuffers::FlatBufferBuilder builder(48);

                auto data = CreateKeepAliveAck(builder, rand);
                auto keep_alive_ack_msg = CreateBladeObjectStoreMessage(
                                                              builder,
                                                              Data_KeepAliveAck,
                                                              data.Union());
                builder.Finish(keep_alive_ack_msg);
                int message_size = builder.GetSize();
                std::memcpy(ctx->send_msg,
                            builder.GetBufferPointer(),
                            message_size);

                send_message(id, message_size);
                break;
            }
        case Data_Sub: {
                uint64_t oid = msg->data_as_Sub()->oid();

                auto data = CreateSubAck(builder, oid);
                auto subscribe_ack_msg = CreateBladeObjectStoreMessage(
                                                            builder,
                                                            Data_SubAck,
                                                            data.Union());

                builder.Finish(subscribe_ack_msg);
                int message_size = builder.GetSize();
                std::memcpy(ctx->send_msg,
                            builder.GetBufferPointer(),
                            message_size);

                send_message(id, message_size);
                
                break;
            }
        case Data_Flush: {
                uint64_t oid = msg->data_as_Flush()->oid();
                auto data = CreateFlushAck(builder, oid);
                auto flush_ack_msg = CreateBladeObjectStoreMessage(builder,
                                                                  Data_FlushAck,
                                                                  data.Union());

                builder.Finish(flush_ack_msg);
                int message_size = builder.GetSize();
                std::memcpy(ctx->send_msg,
                            builder.GetBufferPointer(),
                            message_size);

                send_message(id, message_size);
                break;
            }
        case Data_Lock: {
                uint64_t oid = msg->data_as_Lock()->id();
                auto data = CreateLockAck(builder, oid);
                auto lock_ack_msg = CreateBladeObjectStoreMessage(builder,
                                                                  Data_LockAck,
                                                                  data.Union());

                builder.Finish(lock_ack_msg);
                int message_size = builder.GetSize();
                std::memcpy(ctx->send_msg,
                            builder.GetBufferPointer(),
                            message_size);

                send_message(id, message_size);
                break;
            }
        default:
            LOG<ERROR>("Unknown message");
            exit(-1);
            break;
    }
}

}  // namespace cirrus
