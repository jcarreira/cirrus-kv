/* Copyright 2016 Joao Carreira */

#ifndef _RDMA_CLIENT_H_
#define _RDMA_CLIENT_H_

#include <string>
#include <thread>
#include <memory>
#include <rdma/rdma_cma.h>
#include "src/common/Semaphore.h"

namespace sirius {

/*
 * One GeneralContext for all connections
 */
struct GeneralContext {
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_comp_channel *comp_channel;
    std::thread* cq_poller_thread;
};

// Struct used to carry information from issuing operation
// to receiving completion
// this is allocated when issuing and deallocated
// for async ops this is passed up
struct RDMAOpInfo {
    RDMAOpInfo(struct rdma_cm_id* id_, bool use_s, Semaphore* s) :
        id(id_), use_sem(use_s), op_sem(s) {}

    struct rdma_cm_id* id;
    bool use_sem;
    Semaphore* op_sem;
};

/*
 * One ConnectionContext per connection
 */ 
struct ConnectionContext {
    ConnectionContext() :
        send_msg(0),
        send_msg_mr(0),
        recv_msg(0),
        recv_msg_mr(0),
        peer_addr(0),
        peer_rkey(0) {}
    
    void *send_msg;
    struct ibv_mr *send_msg_mr;

    void* recv_msg;
    struct ibv_mr *recv_msg_mr;

    uint64_t peer_addr;
    uint32_t peer_rkey;

    // these are used for SEND and RECV
    Semaphore send_sem;
    Semaphore recv_sem;
    
    GeneralContext gen_ctx_;
};

class RDMAClient {
public:
    RDMAClient(int timeout_ms = 500);
    ~RDMAClient();
    virtual void connect(const std::string& host, const std::string& port);

protected:
    void build_params(struct rdma_conn_param *params);
    void setup_memory(ConnectionContext& ctx);
    void build_qp_attr(struct ibv_qp_init_attr *qp_attr, ConnectionContext*);
    void build_connection(struct rdma_cm_id *id);
    void build_context(struct ibv_context *verbs, ConnectionContext*);

    // Message (Send/Recv)
    void send_message(rdma_cm_id*, uint64_t size);
    void send_receive_message_sync(rdma_cm_id*, uint64_t size);

    // RDMA (write/read)
    RDMAOpInfo* write_rdma_async(struct rdma_cm_id *id, uint64_t size,
            uint64_t remote_addr, uint64_t peer_rkey);
    void write_rdma_sync(struct rdma_cm_id *id, uint64_t size,
            uint64_t remote_addr, uint64_t peer_rkey);

    RDMAOpInfo* read_rdma_async(struct rdma_cm_id *id, uint64_t size, 
            uint64_t remote_addr, uint64_t peer_rkey);
    void read_rdma_sync(struct rdma_cm_id *id, uint64_t size, 
            uint64_t remote_addr, uint64_t peer_rkey);

    // event poll loop
    static void *poll_cq(ConnectionContext*);
    static void on_completion(struct ibv_wc *wc);

    int post_receive(struct rdma_cm_id *id);

    struct rdma_cm_id *id_;
    struct rdma_event_channel *ec_;

    int timeout_ms_;

    ConnectionContext con_ctx;

    const size_t GB = (1024*1024*1024);
    const size_t RECV_MSG_SIZE = 2 * GB;
    const size_t SEND_MSG_SIZE = 2 * GB;
};

} // sirius

#endif // _RDMA_CLIENT_H_

