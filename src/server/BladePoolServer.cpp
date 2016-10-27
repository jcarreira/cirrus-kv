/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <errno.h>
#include "src/server/BladePoolServer.h"
#include "src/common/BladeMessage.h"
#include "src/common/BladeMessageGenerator.h"
#include "src/utils/easylogging++.h"
#include "src/utils/TimerFunction.h"
#include "src/utils/InfinibandSupport.h"

namespace sirius {

static const int SIZE = 1000000;

BladePoolServer::BladePoolServer(int port,
        uint64_t pool_size,
        int timeout_ms) :
    RDMAServer(port, timeout_ms),
    mr_data_(0),
    mr_pool_(0),
   pool_size_(pool_size) {
}

BladePoolServer::~BladePoolServer() {
}

void BladePoolServer::init() {
    // let upper layer initialize
    // all network related things
    RDMAServer::init();
}

void BladePoolServer::handle_connection(struct rdma_cm_id* id) {
    id = id;
}

void BladePoolServer::handle_disconnection(struct rdma_cm_id* id) {
    id = id;
}

#if 0
uint32_t BladePoolServer::create_pool(uint64_t size, struct rdma_cm_id* id) {
    TimerFunction tf("create_pool", true);

    id = id; // warnings

    LOG(INFO) << "Allocating memory pool of size: " << (size/1024/1024) << "MB "
        << (size/1024/1024/1024) << "GB";

    void *data;
    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&data),
                sysconf(_SC_PAGESIZE), size));
    mr_data_.push_back(data);

    LOG(INFO) << "Creating memory region";
    ibv_mr* pool;
    TEST_Z(pool = ibv_reg_mr(
                gen_ctx_.pd, data, size,
                IBV_ACCESS_LOCAL_WRITE |
                IBV_ACCESS_REMOTE_WRITE |
                IBV_ACCESS_REMOTE_READ));

    mr_pool_.push_back(pool);

    LOG(INFO) << "Memory region created";

    return 0;
}
#else 

uint32_t BladePoolServer::create_pool(uint64_t size, struct rdma_cm_id* id) {
    TimerFunction tf("create_pool");

    LOG(INFO) << "Allocating memory pool of size: " << size;

    void *data;
    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&data),
                sysconf(_SC_PAGESIZE), size));
    mr_data_.push_back(data);

    LOG(INFO) << "Creating memory region";
    ibv_mr* pool;
    TEST_Z(pool = ibv_reg_mr(
                gen_ctx_.pd, data, size,
                IBV_ACCESS_LOCAL_WRITE |
                IBV_ACCESS_REMOTE_WRITE |
                IBV_ACCESS_REMOTE_READ));

    mr_pool_.push_back(pool);

    LOG(INFO) << "Memory region created";
    LOG(INFO) << "Creating memory window";

    InfinibandSupport ibs;
    std::call_once(ibs.mw_support_check_,
            &InfinibandSupport::check_mw_support, &ibs, gen_ctx_.ctx);
    std::call_once(ibs.odp_support_check_,
            &InfinibandSupport::check_odp_support, &ibs, gen_ctx_.ctx);


    // create memory window
    struct ibv_mw* mw;
    TEST_Z(gen_ctx_.pd);

    ibv_pd* pd;
    TEST_Z(pd = ibv_alloc_pd(id->verbs));
    mw = ibv_alloc_mw(pd, IBV_MW_TYPE_2);
    LOG(ERROR) << "errno: " << errno << " EPERM: " << EPERM;
    TEST_Z(mw);
    
    LOG(INFO) << "Creating mw binding config";
    // create configuration
    struct ibv_exp_mw_bind mw_bind;
    std::memset(&mw_bind, 0, sizeof(mw_bind));
    mw_bind.qp = id->qp;
    mw_bind.mw = mw;
    mw_bind.wr_id = 1;
    mw_bind.exp_send_flags = IBV_SEND_SIGNALED;
    mw_bind.bind_info.mr = pool;
    mw_bind.bind_info.addr = reinterpret_cast<uint64_t>(data);
    mw_bind.bind_info.length = size;
    mw_bind.bind_info.exp_mw_access_flags |= IBV_EXP_ACCESS_REMOTE_WRITE
        | IBV_EXP_ACCESS_REMOTE_READ
        | IBV_EXP_ACCESS_LOCAL_WRITE
        | IBV_EXP_ACCESS_MW_BIND;

    LOG(INFO) << "Binding memory window";
    // bind memory window to memory region
    // int ibv_exp_bind_mw(struct ibv_exp_mw_bind *mw_bind);
    int ret;
    TEST_NZ(ret = ibv_exp_bind_mw(&mw_bind));

    LOG(INFO) << "Pool created (and mw bound) successfully";

    return mw->rkey;
}
#endif

void BladePoolServer::process_message(rdma_cm_id* id,
        void* message) {
    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(message);
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    LOG(INFO) << "Received message";

    // we shouldnt do this earlier has soon has process starts
    // but cant make it work like that (yet)
    std::call_once(pool_flag_,
            &BladePoolServer::create_pool, this, pool_size_, id);

    switch (msg->type) {
        case ALLOC:
            {
                LOG(INFO) << "ALLOC";

                //uint64_t size = msg->data.alloc.size;

                uint64_t mr_id = 42;
                uint64_t remote_addr =
                    reinterpret_cast<uint64_t>(mr_data_[0]);
                BladeMessageGenerator::alloc_ack_msg(
                        ctx->send_msg,
                        mr_id,
                        remote_addr,
                        mr_pool_[0]->rkey);

                LOG(INFO) << "Sending ack. "
                    << " remote_addr: " << remote_addr
                    << " rkey: " << mr_pool_[0]->rkey;

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
