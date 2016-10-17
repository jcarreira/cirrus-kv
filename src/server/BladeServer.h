/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_SERVER_H_
#define _BLADE_SERVER_H_

#include "src/utils/utils.h"
#include "RDMAServer.h"
#include <rdma/rdma_cma.h>
#include <vector>
#include <thread>
#include <memory>
#include <semaphore.h>

namespace sirius {

class BladeServer : public RDMAServer {
public:
    BladeServer(int port, int timeout_ms);
    virtual ~BladeServer();

private:
    void process_message(rdma_cm_id*, void* msg);
    void create_pool(uint64_t size);
   
    std::vector<void*> mr_data_;
    std::vector<ibv_mr*> mr_pool_;
};

}

#endif // _BLADE_SERVER_H_
