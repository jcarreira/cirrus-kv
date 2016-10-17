/* Copyright 2016 Joao Carreira */

#ifndef _CONNECTION_CONTEXT_H_
#define _CONNECTION_CONTEXT_H_

#include <rdma/rdma_cma.h>
#include <semaphore.h>

namespace sirius {

class RDMAServer;

class ConnectionContext {
public:
    ConnectionContext();

    // buffer for receiving messages
    void* recv_msg;
    ibv_mr* recv_msg_mr;

    // buffer to send messages
    void* send_msg;
    ibv_mr* send_msg_mr;

    sem_t ack_sem;
   
    // pointer to RDMAServer responsible
    // for handling events on this connection
    RDMAServer* server;

    // checksum
    int checksum = 42;

    // we need a way to distinguish connection contexts
    // an integer is handy
    int context_id;
    static int generateContextId() {
        static uint64_t count = 0;
        return count++;
    }
};

}

#endif // _CONNECTION_CONTEXT_H_
