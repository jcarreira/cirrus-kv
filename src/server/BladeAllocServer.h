#ifndef SRC_SERVER_BLADEALLOCSERVER_H_
#define SRC_SERVER_BLADEALLOCSERVER_H_

#include <rdma/rdma_cma.h>
#include <semaphore.h>
#include <thread>
#include <vector>
#include <set>
#include <map>
#include "utils/utils.h"
#include "RDMAServer.h"

#include <boost/interprocess/managed_external_buffer.hpp>

namespace cirrus {

struct BladeAllocation {
    explicit BladeAllocation(void* p = 0) : ptr(p) {}
    void* ptr;
};

/**
  * This server supports allocations on top of a big mem pool.
  * Allocations are isolated through the use of mem. windows
  * We use a mem. allocator to manage mem.
  */
class BladeAllocServer : public RDMAServer {
 public:
    BladeAllocServer(int port, uint64_t pool_size,
            int timeout_ms = 500);
    virtual ~BladeAllocServer() = default;
    void init() final;

 private:
    void process_message(rdma_cm_id*, void* msg) final;
    void handle_connection(struct rdma_cm_id* id) final;
    void handle_disconnection(struct rdma_cm_id* id) final;

    bool create_pool(uint64_t size);

    void allocate_mem(rdma_cm_id* id, uint64_t size, const void* ptr,
                      const uint32_t& rkey);
    void* find_free_data(uint64_t size);

    // mr and pointer to big pool
    ibv_mr* big_pool_mr_;
    void* big_pool_data_;
    uint64_t big_pool_size_;
    std::once_flag pool_flag_;

    // mrs and pointers to allocations
    // from clients
    // std::set<ibv_mr*> mrs_;
    std::set<void*> mrs_data_;

    std::map<uint64_t, BladeAllocation> id_to_alloc_;
    uint64_t num_allocs_;
    uint64_t mem_allocated;

    std::unique_ptr<boost::interprocess::managed_external_buffer> allocator;
};

}  // namespace cirrus

#endif  // SRC_SERVER_BLADEALLOCSERVER_H_
