#ifndef _RDMA_CLIENT_H_
#define _RDMA_CLIENT_H_

#include <rdma/rdma_cma.h>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <cstring>
#include <atomic>
#include "common/Synchronization.h"
#include "utils/logging.h"

namespace cirrus {

/**
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

/** Struct used to carry information from issuing operation
  * to receiving completion.
  * This is allocated when issuing and deallocated
  * for async ops this is passed up.
  */
struct RDMAOpInfo {
    RDMAOpInfo(struct rdma_cm_id* id_, Lock* s = nullptr,
            std::function<void(void)> fn = []() -> void {}) :
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

/**
  * Struct to maintain connection context. One ConnectionContext per connection
  */
struct ConnectionContext {
    ConnectionContext() :
        send_msg(0), send_msg_mr(0), recv_msg(0),
        recv_msg_mr(0), qp(nullptr),
        peer_addr(0), peer_rkey(0),
        setup_done(false) {
        recv_sem = new SpinLock();
        recv_sem->wait();  // lock
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
    Lock* recv_sem;

    GeneralContext gen_ctx_;
    bool setup_done = false;
};

/**
  * A struct representing a portion of RDMA memory.
  */
struct RDMAMem {
    RDMAMem(const void* addr = nullptr, uint64_t size = 0,
            ibv_mr* m = nullptr) :
        addr_(reinterpret_cast<uint64_t>(addr)), size_(size), mr(m) {}

    ~RDMAMem() {
        if (registered_ && !cleared_) {
            clear();
        }
    }

    /**
      * Registers the RDMA memory
      * @param gctx the GeneralContext for all connections
      * @return Whether memory was successfully registered
      */
    bool prepare(GeneralContext gctx) {
        LOG<INFO>("prepare()");
        // we don't register more than once
        if (registered_) {
            LOG<INFO>("already registered");
            return true;
        }

        mr = ibv_reg_mr(gctx.pd,
                reinterpret_cast<void*>(addr_),
                size_, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);

        if (mr) {
            registered_ = true;
        }
        return mr != nullptr;
    }

    /**
      * Clears the RDMA memory. Calls ibv_dereg_mr() on RDMAMem.mr .
      * @return Whether memory was successfully cleared.
      */
    bool clear() {
        if (registered_ == false)
            throw std::runtime_error("Not registered");

        int ret = ibv_dereg_mr(mr);

        cleared_ = true;

        return ret == 0;
    }

    // we should have a smart pointer around this mr
    uint64_t addr_; /**< Address of the memory */
    uint64_t size_; /**< Size of the memory */
    struct ibv_mr *mr; /**< Pointer to ibv_mr for this memory */
    bool registered_ = false; /**< If the memory is registered. */
    bool cleared_ = false; /**< If the memory is cleared. */
};

/**
  * A class containing the general outline of all clients that use RDMA
  * for communication.
  */
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
    bool send_message(rdma_cm_id*, uint64_t size, Lock* lock = nullptr);
    bool send_receive_message_sync(rdma_cm_id*, uint64_t size);

    // RDMA (write/read)
    RDMAOpInfo* write_rdma_async(struct rdma_cm_id *id, uint64_t size,
            uint64_t remote_addr, uint64_t peer_rkey, const RDMAMem&);
    bool write_rdma_sync(struct rdma_cm_id *id, uint64_t size,
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

    void fetchadd_rdma_sync(struct rdma_cm_id *id,
        uint64_t remote_addr, uint64_t peer_rkey, uint64_t value);
    RDMAOpInfo* fetchadd_rdma_async(struct rdma_cm_id *id,
        uint64_t remote_addr, uint64_t peer_rkey, uint64_t value);

    // event poll loop
    void *poll_cq(ConnectionContext*);
    void on_completion(struct ibv_wc *wc);

    bool post_send(ibv_qp* qp, ibv_send_wr* wr, ibv_send_wr** bad_wr);
    int post_receive(struct rdma_cm_id *id);

    struct rdma_cm_id *id_;
    struct rdma_event_channel *ec_;

    int timeout_ms_;

    ConnectionContext con_ctx_;

    const size_t MB            = (1024 * 1024);
    const size_t GB            = (1024 * MB);
    const size_t RECV_MSG_SIZE = 200 * MB;
    const size_t SEND_MSG_SIZE = 200 * MB;
    const size_t CQ_DEPTH      = 500;
    const size_t MAX_SEND_WR   = 500;
    const size_t MAX_RECV_WR   = 100;
    const size_t MAX_SEND_SGE  = 2;
    const size_t MAX_RECV_SGE  = 2;

    // we use the value in perftest
    static const int MAX_INLINE_DATA = 220;

    std::atomic<uint64_t> outstanding_send_wr;
    std::atomic<uint64_t> outstanding_recv_wr;

    RDMAMem default_recv_mem_;
    RDMAMem default_send_mem_;
};

}  // namespace cirrus

#endif  // _RDMA_CLIENT_H_
