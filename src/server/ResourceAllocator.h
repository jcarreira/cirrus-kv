/* Copyright 2016 Joao Carreira */

#ifndef _RESOURCE_ALLOCATOR_H_
#define _RESOURCE_ALLOCATOR_H_

#include "src/utils/utils.h"
#include "src/server/RDMAServer.h"
#include <rdma/rdma_cma.h>
#include <vector>
#include <thread>
#include <memory>
#include <semaphore.h>

namespace sirius {

class ResourceAllocator : public RDMAServer {
public:
    ResourceAllocator(int port, int timeout_ms = 500);
    virtual ~ResourceAllocator();
private:
    void process_message(rdma_cm_id*, void* msg);
};

} // sirius

#endif // _RESOURCE_ALLOCATOR_H_

