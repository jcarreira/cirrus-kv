/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_OBJECT_STORE_SERVER_H_
#define _BLADE_OBJECT_STORE_SERVER_H_

#include <rdma/rdma_cma.h>
#include "src/utils/utils.h"
#include "RDMAServer.h"
#include <vector>
#include <set>
#include <thread>
#include <semaphore.h>
#include <map>
#include <boost/interprocess/managed_external_buffer.hpp>

namespace sirius {

struct BladeAllocation {
    BladeAllocation(void* p = 0) : ptr(p){}
    void* ptr;
};

class BladeObjectStore : public RDMAServer {
public:
    BladeObjectStore(int port, uint64_t pool_size,
            int timeout_ms = 500);
    virtual ~BladeObjectStore() = default;
    void init() final override;

private:
    void process_message(rdma_cm_id*, void* msg) final override;
    void handle_connection(struct rdma_cm_id* id) final override;
    void handle_disconnection(struct rdma_cm_id* id) final override;

    bool create_pool(uint64_t size);

    void allocate_mem(rdma_cm_id* id, uint64_t size, void*& ptr, uint32_t& rkey);
    void* find_free_data(uint64_t size);

    void checkpoint();

    // mr and pointer to big pool
    ibv_mr* big_pool_mr_;
    void* big_pool_data_;
    uint64_t big_pool_size_;
    std::once_flag pool_flag_;

    std::set<void*> mrs_data_;

    std::map<uint64_t, BladeAllocation> id_to_alloc_;
    uint64_t num_allocs_;
    uint64_t mem_allocated;

    std::unique_ptr<boost::interprocess::managed_external_buffer> allocator;
};

}

#endif // _BLADE_OBJECT_STORE_SERVER_H_
