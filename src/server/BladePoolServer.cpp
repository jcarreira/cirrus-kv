/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <errno.h>
#include "src/server/BladePoolServer.h"
#include "src/common/BladeMessage.h"
#include "src/common/BladeMessageGenerator.h"
#include "src/utils/logging.h"
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

uint32_t BladePoolServer::create_pool(uint64_t size, struct rdma_cm_id* id) {
    TimerFunction tf("create_pool", true);

    id = id;  // warnings

    LOG<INFO>("Allocating memory pool of size: ",
        (size/1024/1024), "MB ",
        (size/1024/1024/1024), "GB");

    void *data;
    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&data),
                static_cast<size_t>(sysconf(_SC_PAGESIZE)), size));
    mr_data_.push_back(data);

    LOG<INFO>("Creating memory region");
    ibv_mr* pool;
    TEST_Z(pool = ibv_reg_mr(
                gen_ctx_.pd, data, size,
                IBV_ACCESS_LOCAL_WRITE |
                IBV_ACCESS_REMOTE_WRITE |
                IBV_ACCESS_REMOTE_READ));

    mr_pool_.push_back(pool);

    LOG<INFO>("Memory region created");

    return 0;
}

void BladePoolServer::process_message(rdma_cm_id* id,
        void* message) {
    auto msg = reinterpret_cast<BladeMessage*>(message);
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    LOG<INFO>("Received message");

    // we should do this as earlier as possible (closer to when)
    // process starts but cant make it work like that (yet)
    std::call_once(pool_flag_,
            &BladePoolServer::create_pool, this, pool_size_, id);

    switch (msg->type) {
        case ALLOC:
            {
                LOG<INFO>("ALLOC");

                // we always return pointer to big memory pool

                uint64_t mr_id = 42;
                uint64_t remote_addr =
                    reinterpret_cast<uint64_t>(mr_data_[0]);
                BladeMessageGenerator::alloc_ack_msg(
                        ctx->send_msg,
                        mr_id,
                        remote_addr,
                        mr_pool_[0]->rkey);

                LOG<INFO>("Sending ack. ",
                    " remote_addr: ", remote_addr,
                    " rkey: ", mr_pool_[0]->rkey);

                // send async message
                send_message(id, sizeof(BladeMessage));

                break;
            }
        case STATS:
                BladeMessageGenerator::stats_msg(ctx->send_msg);
                send_message(id, sizeof(BladeMessage));
            break;
        default:
            LOG<ERROR>("Unknown message");
            exit(-1);
            break;
    }
}

}  // namespace sirius
