/* Copyright 2016 Joao Carreira */

#ifndef _RDMA_CLIENT_H_
#define _RDMA_CLIENT_H_

#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <cstring>
#include <rdma/rdma_cma.h>
#include "src/common/Synchronization.h"
#include "src/utils/logging.h"

namespace sirius {

/*
 * One GeneralContext for all connections
 */
struct GeneralContext {
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_comp_channel *comp_channel;
    struct ibv_device_attr device_attr;
    struct ibv_port_attr port_attr;
    union ibv_gid gid;
    struct ibv_qp_init_attr qp_attr;
    std::thread* cq_poller_thread;
};

// Struct used to carry information from issuing operation
// to receiving completion
// this is allocated when issuing and deallocated
// for async ops this is passed up
struct RDMAOpInfo {
    RDMAOpInfo(struct rdma_cm_id* id_, Lock* s = nullptr,
            std::function<void(void)> fn = []() -> void {}
            ) :
        id(id_), op_sem(s), apply_fn(fn) {
            if (fn == nullptr)
                throw std::runtime_error("BUG");
        }

    void apply() { 
        LOG<INFO>("Applying fn");
        apply_fn();
        LOG<INFO>("Applied fn");
    }

    struct rdma_cm_id* id;
    Lock* op_sem;
    std::function<void(void)> apply_fn;
};

/*
 * One ConnectionContext per connection
 */ 
struct ConnectionContext {
    ConnectionContext() :
        send_msg(0), send_msg_mr(0), recv_msg(0),
        recv_msg_mr(0), qp(nullptr),
        peer_addr(0), peer_rkey(0),
        setup_done(false) 
    {
        recv_sem = new SpinLock();
        recv_sem->wait(); //lock
        memset(&gen_ctx_, 0, sizeof(gen_ctx_));  
    }

    ~ConnectionContext() {
        if (setup_done) {
            ibv_dereg_mr(send_msg_mr);
            ibv_dereg_mr(recv_msg_mr);
        }

        delete recv_sem;
        // XXX release send_msg and recv_msg
    }
    
    void *send_msg;
    struct ibv_mr *send_msg_mr;

    void* recv_msg;
    struct ibv_mr *recv_msg_mr;

    struct ibv_qp* qp;

    uint64_t peer_addr;
    uint32_t peer_rkey;

    // these are used for SEND and RECV
    Lock* send_sem;
    //Semaphore send_sem;
    Lock* recv_sem;
    
    GeneralContext gen_ctx_;
    bool setup_done = false;
};

struct RDMAMem {
    RDMAMem(const void* addr = nullptr, uint64_t size = 0,
            ibv_mr* m = nullptr) :
        addr_(reinterpret_cast<uint64_t>(addr)), size_(size), mr(m) {}

    ~RDMAMem() {
        if (registered_ && !cleared_) {
            clear();
        }
    }

    bool prepare(GeneralContext gctx) {
        mr = ibv_reg_mr(gctx.pd, 
                reinterpret_cast<void*>(addr_),
                size_, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);

        if (mr) {
            registered_ = true;
        }
        return mr != nullptr;
    }
    
    bool clear() {
        if (registered_ == false)
            throw std::runtime_error("Not registered");

        int ret = ibv_dereg_mr(mr);

        cleared_ = true;

        return ret == 0;
    }

    // we should have a smart pointer around this mr
    uint64_t addr_;
    uint64_t size_;
    struct ibv_mr *mr;
    bool registered_ = false;
    bool cleared_ = false;
};

class RDMAClient {
public:
    explicit RDMAClient(int timeout_ms = 500);
    virtual ~RDMAClient();

    virtual void connect(const std::string& host, const std::string& port);

protected:
    void alloc_rdma_memory(ConnectionContext& ctx);

    void build_params(struct rdma_conn_param *params);
    void setup_memory(ConnectionContext& ctx);
    void build_qp_attr(struct ibv_qp_init_attr *qp_attr, ConnectionContext*);
    void build_connection(struct rdma_cm_id *id);
    void build_context(struct ibv_context *verbs, ConnectionContext*);

    // Message (Send/Recv)
    void send_message(rdma_cm_id*, uint64_t size, Lock* lock = nullptr);
    void send_receive_message_sync(rdma_cm_id*, uint64_t size);

    // RDMA (write/read)
    RDMAOpInfo* write_rdma_async(struct rdma_cm_id *id, uint64_t size,
            uint64_t remote_addr, uint64_t peer_rkey, const RDMAMem&);
    void write_rdma_sync(struct rdma_cm_id *id, uint64_t size,
            uint64_t remote_addr, uint64_t peer_rkey, const RDMAMem&);

    RDMAOpInfo* read_rdma_async(struct rdma_cm_id *id, uint64_t size, 
            uint64_t remote_addr, uint64_t peer_rkey,
            const RDMAMem& mem,
            std::function<void()> apply_fn = []() -> void {});
    void read_rdma_sync(struct rdma_cm_id *id, uint64_t size, 
            uint64_t remote_addr, uint64_t peer_rkey,
            const RDMAMem& mem);
    
    void connect_rdma_cm(const std::string& host, const std::string& port);
    void connect_eth(const std::string& host, const std::string& port);

    // event poll loop
    static void *poll_cq(ConnectionContext*);
    static void on_completion(struct ibv_wc *wc);

    int post_receive(struct rdma_cm_id *id);

    struct rdma_cm_id *id_;
    struct rdma_event_channel *ec_;

    int timeout_ms_;

    ConnectionContext con_ctx_;

    const size_t GB            = (1024 * 1024 * 1024);
    const size_t RECV_MSG_SIZE = 2 * GB;
    const size_t SEND_MSG_SIZE = 2 * GB;
    const size_t CQ_DEPTH      = 4 * 1024;
    const size_t MAX_SEND_WR   = 100;
    const size_t MAX_RECV_WR   = 100;
    const size_t MAX_SEND_SGE  = 100;
    const size_t MAX_RECV_SGE  = 100;

    RDMAMem default_recv_mem_;
    RDMAMem default_send_mem_;
};

} // sirius

#endif // _RDMA_CLIENT_H_

