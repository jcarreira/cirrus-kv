/* Copyright 2016 Joao Carreira */

#include <unistd.h>
#include <chrono>
#include "src/server/BladeServer.h"
#include "src/common/BladeMessage.h"
#include "src/common/BladeMessageGenerator.h"
#include "src/utils/easylogging++.h"
#include "src/utils/TimerFunction.h"

namespace sirius {

static const int SIZE = 1000000;

BladeServer::BladeServer(int port, int timeout_ms) :
    RDMAServer(port, timeout_ms) {
}

BladeServer::~BladeServer() {
}

void BladeServer::create_pool(uint64_t size) {
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
}

void BladeServer::process_message(rdma_cm_id* id,
        void* message) {
    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(message);
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    int context_id = ctx->context_id;
    LOG(INFO) << "Received message";

    switch (msg->type) {
        case ALLOC:
            {
                LOG(INFO) << "ALLOC. ctx_id: " << context_id;

                uint64_t size = msg->data.alloc.size;
                create_pool(size);

                uint64_t mr_id = 42;
                uint64_t remote_addr =
                    reinterpret_cast<uint64_t>(mr_data_[context_id]);
                BladeMessageGenerator::alloc_ack_msg(
                        conns_[context_id]->send_msg,
                        mr_id,
                        remote_addr,
                        mr_pool_[context_id]->rkey);

                LOG(INFO) << "Sending ack. "
                    << " remote_addr: " << remote_addr
                    << " rkey: " << mr_pool_[context_id]->rkey;

                send_message(id, sizeof(BladeMessage));

                break;
            }
        //case STATS: // ask for utilization stats
        default:
            LOG(ERROR) << "Unknown message";
            exit(-1);
            break;
    }
}

}
