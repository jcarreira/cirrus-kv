#ifndef _BLADE_POOL_SERVER_H_
#define _BLADE_POOL_SERVER_H_

#include <rdma/rdma_cma.h>
#include <semaphore.h>
#include <thread>
#include <vector>
#include "utils/utils.h"
#include "RDMAServer.h"

/*
 * This server only creates one big pool
 * Allocations here always return a pointer
 * to that big pool
 */

namespace cirrus {

class BladePoolServer : public RDMAServer {
 public:
    BladePoolServer(int port, uint64_t pool_size,
            int timeout_ms = 500);
    virtual ~BladePoolServer() = default;
    void init() final override;

 private:
    void process_message(rdma_cm_id*, void* msg) final override;
    void handle_connection(struct rdma_cm_id* id) final override;
    void handle_disconnection(struct rdma_cm_id* id) final override;

    uint32_t create_pool(uint64_t size, struct rdma_cm_id*);

    std::vector<void*> mr_data_;
    std::vector<ibv_mr*> mr_pool_;
    uint64_t pool_size_;

    std::once_flag pool_flag_;
};

}  // namespace cirrus

#endif // _BLADE_POOL_SERVER_H_
