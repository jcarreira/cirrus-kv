/* Copyright 2016 Joao Carreira */

#ifndef _RDMA_SERVER_H_
#define _RDMA_SERVER_H_

#include <rdma/rdma_cma.h>
#include "src/utils/utils.h"
#include "src/server/Server.h"
#include "src/server/GeneralContext.h"
#include "ConnectionContext.h"
#include <vector>
#include <mutex>
#include <memory>

namespace sirius {
    
class RDMAServer : public Server {
public:
    RDMAServer(int port, int timeout_ms = 500);
    ~RDMAServer();

    // init the server
    virtual void init();

    // get into the event loop
    virtual void loop();

protected:
    // RDMA setup functions
    void build_connection(struct rdma_cm_id *id);
    void build_params(struct rdma_conn_param *params);
    void build_gen_context(struct ibv_context *verbs);
    void build_qp_attr(struct ibv_qp_init_attr *qp_attr);

    void create_connection_context(struct rdma_cm_id *id);
    void handle_disconnected(struct rdma_cm_id *id);
    void handle_established(struct rdma_cm_id *id);
    void exit_destroy();

    static void on_completion(struct ibv_wc *wc);
    static void* poll_cq();
    static void post_msg_receive(struct rdma_cm_id *id);
    static void send_message(struct rdma_cm_id *id, uint64_t size);

    virtual void handle_connection(struct rdma_cm_id*);
    virtual void handle_disconnection(struct rdma_cm_id*);

    // leave this to specific server programs
    virtual void process_message(rdma_cm_id*, void* msg) = 0;

    // connections
    std::vector<ConnectionContext*> conns_;

    // structures used to setup connections
    struct rdma_cm_id *id_ = nullptr;
    struct rdma_event_channel *ec_ = nullptr;

    // port 
    int port_;
    // timeout in miliseconds
    int timeout_ms_;

    // RDMA context for this process
    static GeneralContext gen_ctx_;

    // listen backlog
    int NUM_BACKLOG = 10;
    // size of memory pool
    int SIZE = 10000;

    static const int RECV_MSG_SIZE   = 10000;
    static const int SEND_MSG_SIZE   = 10000;
    // seems to be the maximum inline data
    static const int MAX_INLINE_DATA = 912;

    using msg_handler = void (RDMAServer::*)(rdma_cm_id*, void*);

    std::once_flag gen_ctx_flag;
};

} // sirius

#endif // _RDMA_SERVER_H_
