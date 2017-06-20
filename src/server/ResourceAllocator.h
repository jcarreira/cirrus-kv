#ifndef _RESOURCE_ALLOCATOR_H_
#define _RESOURCE_ALLOCATOR_H_

#include <rdma/rdma_cma.h>
#include <semaphore.h>
#include <thread>
#include <vector>
#include <memory>
#include "utils/utils.h"
#include "server/RDMAServer.h"
#include "authentication/Authenticator.h"
#include "common/AllocatorMessage.h"


namespace cirrus {

class ResourceAllocator : public RDMAServer {
 public:
    explicit ResourceAllocator(int port, int timeout_ms = 500);
    virtual ~ResourceAllocator();

    virtual void init();
 protected:
    void process_message(rdma_cm_id*, void* msg);

    void send_challenge(rdma_cm_id* id, const AllocatorMessage& msg);

    void send_stats(rdma_cm_id* id,
            const AllocatorMessage& msg);

    std::unique_ptr<Authenticator> authenticator_;

    // total amount of memory allocated
    uint64_t total_mem_allocated_;
};

}  // namespace cirrus

#endif  // _RESOURCE_ALLOCATOR_H_
