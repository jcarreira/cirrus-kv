/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <errno.h>
#include <boost/interprocess/creation_tags.hpp>
#include "src/server/BladeObjectStore.h"
#include "src/common/BladeMessage.h"
#include "src/common/BladeObjectStoreMessageGenerator.h"
#include "src/utils/logging.h"
#include "src/utils/Time.h"
#include "src/utils/InfinibandSupport.h"

namespace cirrus {

using BOMG = BladeObjectStoreMessageGenerator;

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
    auto msg = reinterpret_cast<BladeObjectStoreMessage*>(message);
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    LOG<INFO>("Received message");

    // create a big poll
    // and allocate data from here
    std::call_once(pool_flag_,
            &BladeObjectStore::create_pool, this, big_pool_size_);

    switch (msg->type) {
        case ALLOC: {
                uint64_t size = msg->data.alloc.size;

                LOG<INFO>("Received allocation request. size: ", size);
                void* ptr = allocator->allocate(size);

                uint64_t remote_addr =
                    reinterpret_cast<uint64_t>(ptr);

                uint64_t alloc_id = num_allocs_++;
                id_to_alloc_[alloc_id] = BladeAllocation(ptr);

                mrs_data_.insert(ptr);

                BOMG::alloc_ack_msg(
                        ctx->send_msg,
                        alloc_id,
                        remote_addr,
                        big_pool_mr_->rkey);

                LOG<INFO>("Sending ack. ",
                    " remote_addr:", remote_addr);

                // send async message
                send_message(id, sizeof(BladeObjectStoreMessage));

                break;
            }
        case DEALLOC: {
                uint64_t addr = msg->data.dealloc.addr;

                allocator->deallocate(reinterpret_cast<void*>(addr));

                BOMG::dealloc_ack_msg(ctx->send_msg,
                        true);

                LOG<INFO>("Deallocated addr: ", addr);

                // send async message
                send_message(id, sizeof(BladeObjectStoreMessage));

                break;
            }
        case STATS: {
                BOMG::stats_msg(ctx->send_msg);
                send_message(id, sizeof(BladeObjectStoreMessage));
                break;
            }
        case KEEP_ALIVE: {
                uint64_t rand = msg->data.keep_alive.rand;
                BOMG::keep_alive_ack_msg(
                        ctx->send_msg,
                        rand);
                send_message(id, sizeof(BladeObjectStoreMessage));
                break;
            }
        case SUBSCRIBE: {
                uint64_t oid = msg->data.sub.oid;
                BOMG::subscribe_ack_msg(
                        ctx->send_msg,
                        oid);
                send_message(id, sizeof(BladeObjectStoreMessage));
                break;
            }
        case FLUSH: {
                uint64_t oid = msg->data.flush.oid;
                BOMG::flush_ack_msg(
                        ctx->send_msg,
                        oid);
                send_message(id, sizeof(BladeObjectStoreMessage));
                break;
            }
        case LOCK: {
                uint64_t oid = msg->data.lock.id;
                BOMG::lock_ack_msg(
                        ctx->send_msg,
                        oid);
                send_message(id, sizeof(BladeObjectStoreMessage));
                break;
            }
        default:
            LOG<ERROR>("Unknown message");
            exit(-1);
            break;
    }
}

}  // namespace cirrus

