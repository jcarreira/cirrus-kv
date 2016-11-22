/* Copyright 2016 Joao Carreira */

#include "src/server/BladeFileAllocServer.h"
#include <unistd.h>
#include <errno.h>
#include <boost/interprocess/creation_tags.hpp>
#include <string>
#include "src/common/BladeFileMessage.h"
#include "src/common/BladeFileMessageGenerator.h"
#include "src/utils/logging.h"
#include "src/utils/TimerFunction.h"
#include "src/utils/InfinibandSupport.h"

namespace sirius {

static const int SIZE = 1000000;

BladeFileAllocServer::BladeFileAllocServer(int port,
        uint64_t pool_size,
        int timeout_ms) :
    RDMAServer(port, timeout_ms),
    big_pool_mr_(0),
    big_pool_data_(0),
    big_pool_size_(pool_size),
    num_allocs_(0),
    mem_allocated(0) {
}

void BladeFileAllocServer::init() {
    // let upper layer initialize
    // all network related things
    RDMAServer::init();
}

void BladeFileAllocServer::handle_connection(struct rdma_cm_id* id) {
    id = id;
}

void BladeFileAllocServer::handle_disconnection(struct rdma_cm_id* id) {
    id = id;
}

bool BladeFileAllocServer::create_pool(uint64_t size) {
    TimerFunction tf("create_pool");

    LOG<INFO>("Allocating memory pool of size: ", size);

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
    allocator.reset(
        new boost::interprocess::managed_external_buffer(
                boost::interprocess::create_only_t(), big_pool_data_, size));

    return true;
}

void BladeFileAllocServer::process_message(rdma_cm_id* id,
        void* message) {
    auto msg = reinterpret_cast<BladeFileMessage*>(message);
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    LOG<INFO>("Received message");

    // create a big poll
    // and allocate data from here
    std::call_once(pool_flag_,
            &BladeFileAllocServer::create_pool, this, big_pool_size_);

    switch (msg->type) {
    case ALLOC:
        {
           uint64_t size = msg->data.alloc.size;
           std::string filename = msg->data.alloc.filename;

           LOG<INFO>("Received allocation request. ",
                "filename: ", filename,
                "size: ", size);

            if (file_to_alloc_.find(filename) != file_to_alloc_.end()) {
                // file already allocated here
                BladeFileMessageGenerator::alloc_ack_msg(
                       ctx->send_msg,
                       reinterpret_cast<uint64_t>(file_to_alloc_[filename].ptr),
                       big_pool_mr_->rkey);

                LOG<INFO>("File exists. Sending ack. ");
                send_message(id, sizeof(BladeFileMessage));
                return;
            }

            void* ptr = allocator->allocate(size);

            uint64_t remote_addr =
                reinterpret_cast<uint64_t>(ptr);

            uint64_t alloc_id = num_allocs_++;
            file_to_alloc_[filename] = BladeAllocation(ptr);

            BladeFileMessageGenerator::alloc_ack_msg(
                    ctx->send_msg,
                    remote_addr,
                    big_pool_mr_->rkey);

            LOG<INFO>("Sending ack. ",
                " remote_addr: ", remote_addr);

            // send async message
            send_message(id, sizeof(BladeFileMessage));

            break;
        }
    case STATS:
        BladeFileMessageGenerator::stats_msg(ctx->send_msg);
        send_message(id, sizeof(BladeFileMessage));
        break;
    default:
        LOG<ERROR>("Unknown message");
        exit(-1);
        break;
    }
}

}  // namespace sirius

