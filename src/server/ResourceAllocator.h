/* Copyright 2016 Joao Carreira */

#ifndef _RESOURCE_ALLOCATOR_H_
#define _RESOURCE_ALLOCATOR_H_

#include <rdma/rdma_cma.h>
#include "src/utils/utils.h"
#include "src/server/RDMAServer.h"
#include "src/authentication/Authenticator.h"
#include "src/common/AllocatorMessage.h"
#include <vector>
#include <thread>
#include <memory>
#include <semaphore.h>

namespace sirius {

class ResourceAllocator : public RDMAServer {
public:
    ResourceAllocator(int port, int timeout_ms = 500);
    virtual ~ResourceAllocator();
protected:
    void process_message(rdma_cm_id*, void* msg);

    void send_auth_token(rdma_cm_id* id, const AllocatorMessage& msg);
    void send_auth_refusal(rdma_cm_id* id, const AllocatorMessage& msg);

    Authenticator* authenticator_;
};

} // sirius

#endif // _RESOURCE_ALLOCATOR_H_

