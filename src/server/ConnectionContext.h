/* Copyright 2016 Joao Carreira */

#ifndef _CONNECTION_CONTEXT_H_
#define _CONNECTION_CONTEXT_H_

#include <rdma/rdma_cma.h>
#include "src/server/ConnectionInfo.h"
#include <semaphore.h>

namespace sirius {

class RDMAServer;

class ConnectionContext {
public:
    ConnectionContext();
    virtual ~ConnectionContext() = default;

    // buffer and memory region for receiving messages
    void* recv_msg;
    ibv_mr* recv_msg_mr;

    // buffer and memory region to send messages
    void* send_msg;
    ibv_mr* send_msg_mr;

    //sem_t ack_sem;
   
    // pointer to RDMAServer responsible
    // for handling events on this connection
    RDMAServer* server;

    // info about connection
    ConnectionInfo* info;

    // we need a way to distinguish connection contexts
    // an integer is handy
    int context_id_;
    static uint64_t id_count_;
};

}

#endif // _CONNECTION_CONTEXT_H_
