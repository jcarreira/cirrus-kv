#ifndef SRC_CLIENT_RDMACLIENT_H_
#define SRC_CLIENT_RDMACLIENT_H_

#include <rdma/rdma_cma.h>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <cstring>
#include <atomic>
#include <utility>

#include "common/Synchronization.h"
#include "common/AllocationRecord.h"
#include "client/BladeClient.h"
#include "utils/logging.h"

#include "third_party/libcuckoo/src/cuckoohash_map.hh"
#include "third_party/libcuckoo/src/city_hasher.hh"

namespace cirrus {

using ObjectID = uint64_t;

// TODO: should this go here? or be internal to the class?
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

/**
  * An RDMA based client that inherits from BladeClient.
  */
class RDMAClient : public BladeClient {
 public:
    void connect(const std::string& address, const std::string& port) override;
    bool write_sync(ObjectID oid, const void* data, uint64_t size) override;
    bool read_sync(ObjectID oid, void* data, uint64_t size) override;
    cirrus::Future write_async(ObjectID oid, const void* data,
                               uint64_t size) override;
    cirrus::Future read_async(ObjectID oid, void* data, uint64_t size) override;
    bool remove(ObjectID id) override;

 private:
    class FutureBladeOp;
    struct RDMAMem;

    /**
      * A class that stores a size and allocation record.
      */
    class BladeLocation {
     public:
        BladeLocation(uint64_t sz, const AllocationRecord& ar) :
            size(sz), allocRec(ar) {}
        explicit BladeLocation(uint64_t sz = 0) :
            size(sz) {}

        uint64_t size; /**< Size of the allocation */
        AllocationRecord allocRec;
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


    bool readToLocal(BladeLocation loc, void*);

    std::shared_ptr<FutureBladeOp> readToLocalAsync(BladeLocation loc,
            void* ptr);

    bool writeRemote(const void* data, BladeLocation loc, RDMAMem* mem = nullptr);
    std::shared_ptr<FutureBladeOp> writeRemoteAsync(const void *data,
            BladeLocation loc);
    bool insertObjectLocation(ObjectID id,
            uint64_t size, const AllocationRecord& allocRec);

    /**
      * hash to map oid and location.
      * if oid is not found, object is not in store.
      */
    cuckoohash_map<ObjectID, BladeLocation, CityHasher<ObjectID> > objects_;


    // ************************ old blade client *****************************

    /**
      * Information about an async op.
      * A class that contains information about an operation being performed
      * asynchronously.
      */
    class FutureBladeOp {
     public:
        explicit FutureBladeOp(RDMAOpInfo* info) :
            op_info(info) {}

        virtual ~FutureBladeOp() = default;

        void wait();
        bool try_wait();

     private:
        /** op_info for this FutureBladeOp. */
        RDMAOpInfo* op_info;
    };

    AllocationRecord allocate(uint64_t size);
    bool deallocate(const AllocationRecord& ar);

    // writes
    std::shared_ptr<FutureBladeOp> write_async(
            const AllocationRecord& alloc_rec,
            uint64_t offset, uint64_t length,
            const void* data,
            RDMAMem* mem = nullptr);
    bool write_sync(const AllocationRecord& alloc_rec, uint64_t offset,
            uint64_t length, const void* data, RDMAMem* mem = nullptr);

    // XXX We may not need a shared ptr here
    // reads
    std::shared_ptr<FutureBladeOp> rdma_read_async(const AllocationRecord& alloc_rec,
            uint64_t offset, uint64_t length, void *data,
            RDMAMem* mem = nullptr);
    bool rdma_read_sync(const AllocationRecord& alloc_rec, uint64_t offset,
            uint64_t length, void *data, RDMAMem* reg = nullptr);

    // fetch and add atomic
    std::shared_ptr<FutureBladeOp> fetchadd_async(
            const AllocationRecord& alloc_rec,
            uint64_t offset,
            uint64_t value);
    bool fetchadd_sync(const AllocationRecord& alloc_rec, uint64_t offset,
            uint64_t value);

    // *****************From old RDMAClient, above was BladeClient + store ****
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

#endif  // SRC_CLIENT_RDMACLIENT_H_
