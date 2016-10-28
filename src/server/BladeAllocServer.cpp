/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <errno.h>
#include "src/server/BladeAllocServer.h"
#include "src/common/BladeMessage.h"
#include "src/common/BladeMessageGenerator.h"
#include "src/utils/easylogging++.h"
#include "src/utils/TimerFunction.h"
#include "src/utils/InfinibandSupport.h"

namespace sirius {

static const int SIZE = 1000000;

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

BladeAllocServer::~BladeAllocServer() {
}

void BladeAllocServer::init() {
    // let upper layer initialize
    // all network related things
    RDMAServer::init();
}

void BladeAllocServer::handle_connection(struct rdma_cm_id* id) {
    id = id;
}

void BladeAllocServer::handle_disconnection(struct rdma_cm_id* id) {
    id = id;
}

bool BladeAllocServer::create_pool(uint64_t size) {
    TimerFunction tf("create_pool");

    LOG(INFO) << "Allocating memory pool of size: " << size;

    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&big_pool_data_),
                sysconf(_SC_PAGESIZE), size));

    LOG(INFO) << "Creating memory region";
    TEST_Z(big_pool_mr_ = ibv_reg_mr(
                gen_ctx_.pd, big_pool_data_, size,
                IBV_ACCESS_LOCAL_WRITE |
                IBV_ACCESS_REMOTE_WRITE |
                IBV_ACCESS_REMOTE_READ));

    LOG(INFO) << "Memory region created";

    return true;
}

void* BladeAllocServer::find_free_data(uint64_t size) {
    size = size;
    // we start allocating from big_pool_data onwards
    return reinterpret_cast<void*>(
            reinterpret_cast<char*>(big_pool_data_) + mem_allocated);
}

void BladeAllocServer::allocate_mem(rdma_cm_id* id,
        uint64_t size, void*& ptr, uint32_t& rkey) {
    ptr = find_free_data(size);

    // // create memory window
    // struct ibv_mw* mw;
    // TEST_Z(mw = ibv_alloc_mw(gen_ctx_.pd, IBV_MW_TYPE_1));

    // LOG(INFO) << "Creating mw binding config";
    // // create configuration
    // struct ibv_exp_mw_bind mw_bind;
    // std::memset(&mw_bind, 0, sizeof(mw_bind));
    // mw_bind.qp = id->qp;
    // mw_bind.mw = mw;
    // mw_bind.wr_id = 1;
    // mw_bind.exp_send_flags = IBV_SEND_SIGNALED;
    // mw_bind.bind_info.mr = big_pool_mr_;
    // mw_bind.bind_info.addr = reinterpret_cast<uint64_t>(ptr);
    // mw_bind.bind_info.length = size;
    // mw_bind.bind_info.exp_mw_access_flags |= IBV_EXP_ACCESS_REMOTE_WRITE
    //     | IBV_EXP_ACCESS_REMOTE_READ
    //     | IBV_EXP_ACCESS_LOCAL_WRITE
    //     | IBV_EXP_ACCESS_MW_BIND;

    // LOG(INFO) << "Binding memory window";

    // // bind memory window to memory region
    // // int ibv_exp_bind_mw(struct ibv_exp_mw_bind *mw_bind);
    // int ret;
    // TEST_NZ(ret = ibv_exp_bind_mw(&mw_bind));
    // rkey = mw->rkey;
    rkey = big_pool_mr_->rkey;
}

void BladeAllocServer::process_message(rdma_cm_id* id,
        void* message) {
    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(message);
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    LOG(INFO) << "Received message";

    // create a big poll
    // and allocate data from here
    std::call_once(pool_flag_,
            &BladeAllocServer::create_pool, this, big_pool_size_);

    switch (msg->type) {
        case ALLOC:
            {
                uint64_t size = msg->data.alloc.size;

                LOG(INFO) << "Received allocation request. size: " << size;

                void* ptr;
                uint32_t rkey;
                allocate_mem(id, size, ptr, rkey);
                mem_allocated += size;

                uint64_t remote_addr =
                    reinterpret_cast<uint64_t>(ptr);

                uint64_t alloc_id = num_allocs_++;
                id_to_alloc_[alloc_id] = BladeAllocation(ptr);

                mrs_data_.insert(ptr);

                BladeMessageGenerator::alloc_ack_msg(
                        ctx->send_msg,
                        alloc_id,
                        remote_addr,
                        rkey);

                LOG(INFO) << "Sending ack. "
                    << " remote_addr: " << remote_addr
                    << " rkey: " << rkey;

                // send async message
                send_message(id, sizeof(BladeMessage));

                break;
            }
        case STATS:
                BladeMessageGenerator::stats_msg(ctx->send_msg);
                send_message(id, sizeof(BladeMessage));
            break;
        default:
            LOG(ERROR) << "Unknown message";
            exit(-1);
            break;
    }
}

}  // namespace sirius

