#ifndef SRC_CLIENT_RDMACLIENT_H_
#define SRC_CLIENT_RDMACLIENT_H_

#include <rdma/rdma_cma.h>
#include <unistd.h>
#include <infiniband/verbs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <cstring>
#include <atomic>
#include <utility>
#include <random>
#include <cassert>

#include "common/ThreadPinning.h"
#include "common/Exception.h"
#include "common/schemas/BladeMessage_generated.h"
#include "common/Synchronization.h"
#include "common/AllocationRecord.h"
#include "common/Serializer.h"
#include "client/BladeClient.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include "utils/CirrusTime.h"

#include "third_party/libcuckoo/src/cuckoohash_map.hh"
#include "third_party/libcuckoo/src/city_hasher.hh"


namespace cirrus {

using ObjectID = uint64_t;
static const int initial_buffer_size = 50;


// TODO(Tyler): should this go here? or be internal to the class?
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
template<class T>
class RDMAClient : public BladeClient<T> {
 public:
    void connect(const std::string& address, const std::string& port) override;
    bool write_sync(ObjectID id,  const T& obj,
        const Serializer<T>& serializer) override;
    bool read_sync(ObjectID oid, void* data, uint64_t size) override;

    // TODO(Tyler): Add async
    // cirrus::Future write_async(ObjectID oid, const void* data,
    //                            uint64_t size) override;
    // cirrus::Future read_async(ObjectID oid,
    //     void* data, uint64_t size) override;
    bool remove(ObjectID id) override;

 private:
    /**
     * The seed used for calls to rand_r. Set based on system time
     * during call to connect.
     */
    unsigned int seed;
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

    /**
     * Struct used to carry information from issuing operation
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
      * Struct to maintain connection context.
      * One ConnectionContext is used per connection.
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
            if (mr == nullptr) {
                if (errno == EINVAL) {
                    LOG<ERROR>("EINVAL from ibv_reg_mr");
                } else if (errno == ENOMEM) {
                    LOG<ERROR>("ENOMEM from ibv_reg_mr");
                } else {
                    LOG<ERROR>("ibv_reg_mr failed, are you trying to write "
                                   "from a string literal?");
                }
            }
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

    bool writeRemote(const void* data, BladeLocation loc,
        RDMAMem* mem = nullptr);
    std::shared_ptr<FutureBladeOp> writeRemoteAsync(const void *data,
            BladeLocation loc);
    bool insertObjectLocation(ObjectID id,
            uint64_t size, const AllocationRecord& allocRec);

    /**
      * hash to map oid and location.
      * if oid is not found, object is not in store.
      */
    cuckoohash_map<ObjectID, BladeLocation, CityHasher<ObjectID> > objects_;

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
    std::shared_ptr<FutureBladeOp> rdma_write_async(
            const AllocationRecord& alloc_rec,
            uint64_t offset, uint64_t length,
            const void* data,
            RDMAMem* mem = nullptr);
    bool rdma_write_sync(const AllocationRecord& alloc_rec, uint64_t offset,
            uint64_t length, const void* data, RDMAMem* mem = nullptr);

    // XXX We may not need a shared ptr here
    // reads
    std::shared_ptr<FutureBladeOp> rdma_read_async(
        const AllocationRecord& alloc_rec,
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

    void alloc_rdma_memory(ConnectionContext *ctx);

    void build_params(struct rdma_conn_param *params);
    void setup_memory(ConnectionContext *ctx);
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
    /** Maximum size of message that can be received at once. */
    const size_t RECV_MSG_SIZE = 200 * MB;
    /** Maximum size of message that can be sent at once. */
    const size_t SEND_MSG_SIZE = 200 * MB;
    /** Maximum number of items in completion queue. */
    const size_t CQ_DEPTH      = 500;
    /** Maximum number of items in send queue. */
    const size_t MAX_SEND_WR   = 500;
    /** Maximum number of items in receive queue. */
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

/**
  * Connects the client to the remote server.
  */
template<class T>
void RDMAClient<T>::connect(const std::string& host, const std::string& port) {
  seed = time(nullptr);
  connect_rdma_cm(host, port);
}

/**
  * Asynchronously writes an object to remote storage under id.
  * @param id the id of the object the user wishes to write to remote memory.
  * @param data a pointer to the buffer where the serialized object should
  * be read read from.
  * @param size the size of the serialized object being read from
  * local memory.
  * @return A Future that contains information about the status of the
  * operation.
  */

  // TODO(Tyler): implement async method
// cirrus::Future RDMAClient::write_async(ObjectID oid,
//                                        void* data, uint64_t size) {
//     // Make sure that the pointer is not null
//     TEST_NZ(data == nullptr);
//     // Create flatbuffer builder
//     std::shared_ptr<flatbuffers::FlatBufferBuilder> builder =
//                             std::make_shared<flatbuffers::FlatBufferBuilder>(
//                                 initial_buffer_size);
//
//     // Create and send write request
//     int8_t *data_cast = reinterpret_cast<int8_t*>(data);
//     std::vector<int8_t> data_vector(data_cast, data_cast + size);
//     auto data_fb_vector = builder->CreateVector(data_vector);
//     auto msg_contents = message::BladeMessage::CreateWrite(*builder,
//                                                               oid,
//                                                               data_fb_vector);
//     auto msg = message::BladeMessage::CreateBladeMessage(
//                                         *builder,
//                                         curr_txn_id,
//                                         message::BladeMessage::Message_Write,
//                                         msg_contents.Union());
//     builder->Finish(msg);
//
//     return enqueue_message(builder);
// }

/**
  * Asynchronously reads an object corresponding to ObjectID
  * from the remote server.
  * @param id the id of the object the user wishes to read to local memory.
  * @param data a pointer to the buffer where the serialized object should
  * be read to.
  * @param size the size of the serialized object being read from
  * remote storage.
  * @return True if the object was successfully read from the server, false
  * otherwise.
  */

// TODO(Tyler): ADD ASYNC
// cirrus::Future RDMAClient::read_async(ObjectID oid, void* data,
//                                      uint64_t /* size */) {
//     std::shared_ptr<flatbuffers::FlatBufferBuilder> builder =
//                             std::make_shared<flatbuffers::FlatBufferBuilder>(
//                                 initial_buffer_size);
//
//     // Create and send read request
//
//     auto msg_contents = message::BladeMessage::CreateRead(*builder, oid);
//
//     auto msg = message::BladeMessage::CreateBladeMessage(
//                                         *builder,
//                                         curr_txn_id,
//                                         message::BladeMessage::Message_Read,
//                                         msg_contents.Union());
//     builder->Finish(msg);
//
//     return enqueue_message(builder, data);
// }

/**
  * Writes an object to remote storage under id.
  * @param id the id of the object the user wishes to write to remote memory.
  * @param data a pointer to the buffer where the serialized object should
  * be read read from.
  * @param size the size of the serialized object being read from
  * local memory.
  * @return True if the object was successfully written to the server, false
  * otherwise.
  */
template<class T>
bool RDMAClient<T>::write_sync(ObjectID oid,  const T& obj,
    const Serializer<T>& serializer) {
    bool retval;
    BladeLocation loc;

    // allocate buffer to serialize into
    auto size = serializer.size(obj);
    std::unique_ptr<char[]> ptr(new char[size]);
    auto data = ptr.get();
    // serialize into the buffer
    serializer.serialize(obj, data);
    if (objects_.find(oid, loc)) {
        retval = writeRemote(data, loc, nullptr);
    } else {
        // we could merge this into a single message (?)
        cirrus::AllocationRecord allocRec;
        {
            TimerFunction tf("FullBladeObjectStoreTempl::put allocate", true);
            allocRec = allocate(size);
        }
        insertObjectLocation(oid, size, allocRec);
        LOG<INFO>("FullBladeObjectStoreTempl::writeRemote after alloc");
        retval = writeRemote(data,
                          BladeLocation(size, allocRec),
                          nullptr);
    }
    return retval;
}

/**
  * Reads an object corresponding to ObjectID from the remote server.
  * @param id the id of the object the user wishes to read to local memory.
  * @param data a pointer to the buffer where the serialized object should
  * be read to.
  * @param size the size of the serialized object being read from
  * remote storage.
  * @return True if the object was successfully read from the server, false
  * otherwise.
  */
template<class T>
bool RDMAClient<T>::read_sync(ObjectID oid, void* data, uint64_t /* size */) {
    BladeLocation loc;
    if (objects_.find(oid, loc)) {
        // Read into the section of memory you just allocated
        readToLocal(loc, data);
        return true;
    } else {
        throw cirrus::NoSuchIDException("Requested ObjectID "
                                        "does not exist remotely.");
    }
    return false;
}

/**
  * Removes the object corresponding to the given ObjectID from the
  * remote store.
  * @param oid the ObjectID of the object to be removed.
  * @return True if the object was successfully removed from the server, false
  * if the object does not exist remotely or if another error occurred.
  */
template<class T>
bool RDMAClient<T>::remove(ObjectID oid) {
    BladeLocation loc;
    if (objects_.find(oid, loc)) {
        objects_.erase(oid);
        return deallocate(loc.allocRec);
    } else {
        throw cirrus::NoSuchIDException("Error. Trying to remove "
                                        "nonnexistent object");
    }
    return false;
}

/**
 * Reads an object to local memory from the remote store.
 * @param loc the BladeLocation for the desired object. Contains the
 * address and size of the object.
 * @param ptr a pointer to the memory where the object will be read to.
 */
template<class T>
bool RDMAClient<T>::readToLocal(BladeLocation loc, void* ptr) {
    RDMAMem mem(ptr, loc.size);
    rdma_read_sync(loc.allocRec, 0, loc.size, ptr, &mem);
    return true;
}

/**
 * Reads an object to local memory from the remote store asynchronously.
 * @param loc the BladeLocation for the desired object. Contains the
 * address and size of the object.
 * @param ptr a pointer to the memory where the object will be read to.
 * @return a std::shared_ptr to a FutureBladeOp. This FutureBladeOp
 * contains information about the status of the operation.
 */
template<class T>
std::shared_ptr<typename RDMAClient<T>::FutureBladeOp>
RDMAClient<T>::readToLocalAsync(BladeLocation loc, void* ptr) {
    auto future = rdma_read_async(loc.allocRec, 0, loc.size, ptr);
    return future;
}

/**
 * Writes an object from local memory to the remote store.
 * @param data a pointer to the data to be written.
 * @param loc a BladeLocation, containing the size of the object and
 * the location to be written to.
 * @param mem a pointer to an RDMAMem, which is the registered memory
 * where the data currently resides. If null, a new RDMAMem is used
 * for this object.
 */
template<class T>
bool RDMAClient<T>::writeRemote(const void *data, BladeLocation loc,
                             RDMAMem* mem) {
    RDMAMem mm(data, loc.size);
    {
        // TimerFunction tf("writeRemote", true);
        // LOG<INFO>("FullBladeObjectStoreTempl:: writing sync");
        rdma_write_sync(loc.allocRec, 0, loc.size, data,
                nullptr == mem ? &mm : mem);
    }
    return true;
}

/**
 * Writes an object from local memory to the remote store asynchronously.
 * @param data a pointer to the data to be written.
 * @param loc a BladeLocation, containing the size of the object and
 * the location to be written to.
 * @param mem a pointer to an RDMAMem, which is the registered memory
 * where the data currently resides. If null, a new RDMAMem is used
 * for this object.
 * @return a std::shared_ptr to a FutureBladeOp. This FutureBladeOp
 * contains information about the status of the operation.
 */
template<class T>
std::shared_ptr<typename RDMAClient<T>::FutureBladeOp>
RDMAClient<T>::writeRemoteAsync(const void *data, BladeLocation loc) {
    auto future = rdma_write_async(loc.allocRec, 0, loc.size, data);
    return future;
}

/**
 * Inserts an object into objects_ , which maps ObjectIDs to
 * BladeLocation objects.
 * @param id the ObjectID of the object.
 * @param size the size of the object.
 * @param allocRec the AllocationRecord being used for this object.
 */
template<class T>
bool RDMAClient<T>::insertObjectLocation(ObjectID id,
                                      uint64_t size,
                                      const AllocationRecord& allocRec) {
    objects_[id] = BladeLocation(size, allocRec);
    return true;
}

/**
  * Initializes rdma params.
  * This method takes a pointer to a struct rdma_conn_param and assigns initial
  * values.
  * @param params the struct rdma_conn_param to be initialized.
  */
template<class T>
void RDMAClient<T>::build_params(struct rdma_conn_param *params) {
    memset(params, 0, sizeof(*params));
    params->initiator_depth = params->responder_resources = 10;
    params->rnr_retry_count = 70;
    params->retry_count = 7;
}

template<class T>
void RDMAClient<T>::alloc_rdma_memory(ConnectionContext *ctx) {
    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&ctx->recv_msg),
                sysconf(_SC_PAGESIZE),
                RECV_MSG_SIZE));
    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&con_ctx_.send_msg),
                static_cast<size_t>(sysconf(_SC_PAGESIZE)),
                SEND_MSG_SIZE));
}

template<class T>
void RDMAClient<T>::setup_memory(ConnectionContext *ctx) {
    alloc_rdma_memory(ctx);

    LOG<INFO>("Registering region with size: ",
            (RECV_MSG_SIZE / 1024 / 1024), " MB");
    TEST_Z(con_ctx_.recv_msg_mr =
            ibv_reg_mr(ctx->gen_ctx_.pd, con_ctx_.recv_msg,
                RECV_MSG_SIZE,
                IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));

    TEST_Z(con_ctx_.send_msg_mr =
            ibv_reg_mr(ctx->gen_ctx_.pd, con_ctx_.send_msg,
                SEND_MSG_SIZE,
                IBV_ACCESS_LOCAL_WRITE |
                IBV_ACCESS_REMOTE_WRITE));

    default_recv_mem_ = RDMAMem(ctx->recv_msg,
            RECV_MSG_SIZE, con_ctx_.recv_msg_mr);
    default_send_mem_ = RDMAMem(ctx->send_msg,
            SEND_MSG_SIZE, con_ctx_.send_msg_mr);

    con_ctx_.setup_done = true;
    LOG<INFO>("Set up memory done");
}

template<class T>
void RDMAClient<T>::build_qp_attr(struct ibv_qp_init_attr *qp_attr,
        ConnectionContext* ctx) {
    memset(qp_attr, 0, sizeof(*qp_attr));

    qp_attr->send_cq = ctx->gen_ctx_.cq;
    qp_attr->recv_cq = ctx->gen_ctx_.cq;
    qp_attr->qp_type = IBV_QPT_RC;

    qp_attr->cap.max_send_wr = MAX_SEND_WR;
    qp_attr->cap.max_recv_wr = MAX_RECV_WR;
    qp_attr->cap.max_send_sge = MAX_SEND_SGE;
    qp_attr->cap.max_recv_sge = MAX_RECV_SGE;
}

/**
 * Posts a work request to the receive queue.
 * @param id a pointer to the rdma_cm_id that contains the information
 * needed to construct the receive request.
 */
template<class T>
int RDMAClient<T>::post_receive(struct rdma_cm_id *id) {
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    LOG<INFO>("Posting receive");

    struct ibv_recv_wr wr, *bad_wr = nullptr;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    auto op_info = new RDMAClient::RDMAOpInfo(id, ctx->recv_sem);
    wr.wr_id = reinterpret_cast<uint64_t>(op_info);
    wr.sg_list = &sge;
    wr.num_sge = 1;

    sge.addr = reinterpret_cast<uint64_t>(ctx->recv_msg);
    sge.length = 1000;  // FIX
    sge.lkey = ctx->recv_msg_mr->lkey;

    return ibv_post_recv(id->qp, &wr, &bad_wr);
}

template<class T>
void RDMAClient<T>::build_connection(struct rdma_cm_id *id) {
    struct ibv_qp_init_attr qp_attr;

    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    build_context(id->verbs, ctx);
    build_qp_attr(&qp_attr, ctx);

    // create the QP we are going to use to communicate with
    TEST_NZ(rdma_create_qp(id, ctx->gen_ctx_.pd, &qp_attr));
}

template<class T>
void RDMAClient<T>::build_context(struct ibv_context *verbs,
        ConnectionContext* ctx) {
    // FIX: we only need one context?
    ctx->gen_ctx_.ctx = verbs;

    TEST_Z(ctx->gen_ctx_.pd = ibv_alloc_pd(ctx->gen_ctx_.ctx));
    TEST_Z(ctx->gen_ctx_.comp_channel =
            ibv_create_comp_channel(ctx->gen_ctx_.ctx));
    TEST_Z(ctx->gen_ctx_.cq = ibv_create_cq(ctx->gen_ctx_.ctx,
                CQ_DEPTH, nullptr, ctx->gen_ctx_.comp_channel, 0));
    TEST_NZ(ibv_req_notify_cq(ctx->gen_ctx_.cq, 0));

    ctx->gen_ctx_.cq_poller_thread =
        new std::thread(&RDMAClient::poll_cq, this, ctx);

    unsigned n_threads = std::thread::hardware_concurrency();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> rv(1, n_threads);

    ThreadPinning::pinThread(ctx->gen_ctx_.cq_poller_thread->native_handle(),
            rv(gen));  // random core
}

/**
 * Polls the completion queue, monitoring it for any event completions.
 * If a request is completed successfully, it is acted upon.
 * @param ctx a pointer to the ConnectionContext that should be monitored.
 */
template<class T>
void* RDMAClient<T>::poll_cq(ConnectionContext* ctx) {
    struct ibv_cq *cq;
    struct ibv_wc wc;
    void* cq_ctx;

    while (1) {
        TEST_NZ(ibv_get_cq_event(ctx->gen_ctx_.comp_channel, &cq, &cq_ctx));
        ibv_ack_cq_events(cq, 1);
        TEST_NZ(ibv_req_notify_cq(cq, 0));

        while (ibv_poll_cq(cq, 1, &wc)) {
            if (wc.status == IBV_WC_SUCCESS) {
                on_completion(&wc);
            } else {
                std::cerr <<
                    "poll_cq: status is not IBV_WC_SUCCESS" << std::endl;
                std::cerr << "wr_id: " << wc.wr_id << std::endl;
                std::cerr << "status: " << wc.status << std::endl;
                DIE("Leaving..");
            }
        }
    }

    return nullptr;
}

/**
 * Method that handles ibv Work completion structs. Signals the op_sem
 * attached to the op_info that tracks the ibv_wc.
 * @param wc a pointer to the ibv_wc to be handled.
 */
template<class T>
void RDMAClient<T>::on_completion(struct ibv_wc *wc) {
    RDMAClient::RDMAOpInfo* op_info = reinterpret_cast<RDMAClient::RDMAOpInfo*>(
                                                                    wc->wr_id);

    LOG<INFO>("on_completion. wr_id: ", wc->wr_id,
        " opcode: ", wc->opcode,
        " byte_len: ", wc->byte_len);

    op_info->apply();

    switch (wc->opcode) {
        case IBV_WC_RECV:
            if (op_info->op_sem)
                op_info->op_sem->signal();
            break;
        case IBV_WC_RDMA_READ:
        case IBV_WC_RDMA_WRITE:
            assert(outstanding_send_wr > 0);
            outstanding_send_wr--;
            if (op_info->op_sem)
                op_info->op_sem->signal();
            break;
        case IBV_WC_SEND:
            assert(outstanding_send_wr > 0);
            outstanding_send_wr--;
            if (op_info->op_sem)
                op_info->op_sem->signal();
            break;
        default:
            LOG<ERROR>("Unknown opcode");
            exit(-1);
            break;
    }
}

/**
  * Sends + Receives synchronously.
  * This function sends a message and then waits for a reply.
  * @param id a pointer to a struct rdma_cm_id that holds the message to send
  * @param size the size of the message being sent
  * @return false if message fails to send, true otherwise. Only returns
  * once a reply is received.
  */
template<class T>
bool RDMAClient<T>::send_receive_message_sync(struct rdma_cm_id *id,
        uint64_t size) {
    auto con_ctx = reinterpret_cast<ConnectionContext*>(id->context);

    // post receive. We are going to receive a reply
    TEST_NZ(post_receive(id_));

    // send our RPC

    Lock* l = new SpinLock();
    if (!send_message(id, size, l))
        return false;

    // wait for SEND completion
    l->wait();

    LOG<INFO>("Sent is done. Waiting for receive");

    // Wait for reply
    con_ctx->recv_sem->wait();

    return true;
}

/**
  * Sends an RDMA message.
  * This function sends a message using RDMA.
  * @param id a pointer to a struct rdma_cm_id that holds the message to send
  * @param size the size of the message being sent
  * @param lock lock used to create an RDMAOpInfo, then passed to
  * a struct ibv_send_wr, which is passed to post_send()
  * @return success of a call to post_send()
  */
template<class T>
bool RDMAClient<T>::send_message(struct rdma_cm_id *id, uint64_t size,
        Lock* lock) {
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    struct ibv_send_wr wr, *bad_wr = nullptr;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    if (lock != nullptr)
        lock->wait();

    auto op_info  = new RDMAClient::RDMAOpInfo(id, lock);
    wr.wr_id      = reinterpret_cast<uint64_t>(op_info);
    wr.opcode     = IBV_WR_SEND;
    wr.sg_list    = &sge;
    wr.num_sge    = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    // XXX doesn't work for now
    // if (size <= MAX_INLINE_DATA)
    //    wr.send_flags |= IBV_SEND_INLINE;

    sge.addr   = reinterpret_cast<uint64_t>(ctx->send_msg);
    sge.length = size;
    sge.lkey   = ctx->send_msg_mr->lkey;

    return post_send(id->qp, &wr, &bad_wr);
}


/**
  * Writes over RDMA synchronously.
  * This function writes synchronously using RDMA. It will not return
  * until the message is sent.
  * @param id a pointer to a struct rdma_cm_id that holds the message to send
  * @param size the size of the message being sent
  * @param remote_addr the remote address to write to
  * @param peer_rkey
  * @param mem a reference to an RDMAMem.
  * @return Success.
  */
template<class T>
bool RDMAClient<T>::write_rdma_sync(struct rdma_cm_id *id, uint64_t size,
        uint64_t remote_addr, uint64_t peer_rkey, const RDMAMem& mem) {
    LOG<INFO>("RDMAClient:: write_rdma_async");
    auto op_info = write_rdma_async(id, size, remote_addr, peer_rkey, mem);
    LOG<INFO>("RDMAClient:: waiting");

    if (nullptr == op_info) {
        throw std::runtime_error("Error writing rdma async");
    }

    {
        TimerFunction tf("waiting semaphore", true);
        op_info->op_sem->wait();
    }

    delete op_info->op_sem;
    delete op_info;

    return true;
}

/**
  * Posts send request.
  * This function calls the function ibv_post_send() to post a wr (work request)
  * to the send queue.
  * @param qp the qp of the struct rdma_cm_id
  * @param wr the work request to post
  * @param bad_wr address of a null pointer
  * @return Success of call to ibv_post_send().
  */
template<class T>
bool RDMAClient<T>::post_send(ibv_qp* qp, ibv_send_wr* wr,
    ibv_send_wr** bad_wr) {
    if (outstanding_send_wr == MAX_SEND_WR) {
        return false;
    }

    outstanding_send_wr++;
    if (ibv_post_send(qp, wr, bad_wr)) {
        LOG<ERROR>("Error post_send.",
            " errno: ", errno);
    }

    return true;
}


/**
  * Writes over RDMA asynchronously.
  * This function writes asynchronously using RDMA. It will return immediately
  * after posting the send request
  * @param id a pointer to a struct rdma_cm_id that holds the message to send
  * @param size the size of the message being sent
  * @param remote_addr the remote address to write to
  * @param peer_rkey
  * @param mem a reference to an RDMAMem.
  * @return A pointer to an RDMAOpInfo containing information about the status
  * of the write.
  */
template<class T>
typename RDMAClient<T>::RDMAOpInfo* RDMAClient<T>::write_rdma_async(
                                                    struct rdma_cm_id *id,
                                                    uint64_t size,
                                                    uint64_t remote_addr,
                                                    uint64_t peer_rkey,
                                                    const RDMAMem& mem) {
#if __GNUC__ >= 7
    [[maybe_unused]]
#else
    __attribute__((unused))
#endif
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    struct ibv_send_wr wr, *bad_wr = nullptr;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));
    memset(&sge, 0, sizeof(sge));

    Lock* l = new SpinLock();
    l->wait();

    auto op_info = new RDMAClient::RDMAOpInfo(id, l);
    wr.wr_id = reinterpret_cast<uint64_t>(op_info);
    wr.opcode = IBV_WR_RDMA_WRITE;
    wr.wr.rdma.remote_addr = remote_addr;
    wr.wr.rdma.rkey = peer_rkey;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    // XXX doesn't work for now
    // if (size <= MAX_INLINE_DATA)
    //    wr.send_flags |= IBV_SEND_INLINE;

    sge.addr   = mem.addr_;
    sge.length = size;
    sge.lkey   = mem.mr->lkey;

    if (!post_send(id->qp, &wr, &bad_wr))
        throw std::runtime_error("write_rdma_async: error post_send");

    return op_info;
}

/**
  * Reads over RDMA synchronously.
  * This function reads synchronously using RDMA. It will not return
  * until the read is complete.
  * @param id a pointer to a struct rdma_cm_id that holds the message to send
  * @param size the size of the message being sent
  * @param remote_addr the remote address to write to
  * @param peer_rkey
  * @param mem a reference to an RDMAMem.
  */
template<class T>
void RDMAClient<T>::read_rdma_sync(struct rdma_cm_id *id, uint64_t size,
        uint64_t remote_addr, uint64_t peer_rkey, const RDMAMem& mem) {
        RDMAClient::RDMAOpInfo* op_info = read_rdma_async(id, size,
            remote_addr, peer_rkey, mem);

    // wait until operation is completed

    {
        TimerFunction tf("read_rdma_async wait for lock", true);
        op_info->op_sem->wait();
    }

    delete op_info->op_sem;
    delete op_info;
}


/**
  * Reads over RDMA asynchronously.
  * This function reads asynchronously using RDMA. It will return immediately
  * after posting the read request
  * @param id a pointer to a struct rdma_cm_id that holds the message to send
  * @param size the size of the message being sent
  * @param remote_addr the remote address to read from
  * @param peer_rkey
  * @param mem a reference to an RDMAMem.
  * @return A pointer to an RDMAOpInfo containing information about the status
  * of the read.
  */
template<class T>
typename RDMAClient<T>::RDMAOpInfo* RDMAClient<T>::read_rdma_async(
                                            struct rdma_cm_id *id,
                                            uint64_t size,
                                            uint64_t remote_addr,
                                            uint64_t peer_rkey,
                                            const RDMAMem& mem,
                                            std::function<void()> apply_fn) {
#if __GNUC__ >= 7
    [[maybe_unused]]
#else
    __attribute__((unused))
#endif
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    struct ibv_send_wr wr, *bad_wr = nullptr;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));
    memset(&sge, 0, sizeof(sge));

    Lock* l = new SpinLock();
    l->wait();

    auto op_info    = new RDMAClient::RDMAOpInfo(id, l, apply_fn);
    wr.wr_id        = reinterpret_cast<uint64_t>(op_info);
    wr.opcode       = IBV_WR_RDMA_READ;
    wr.wr.rdma.remote_addr = remote_addr;
    wr.wr.rdma.rkey = peer_rkey;
    wr.sg_list      = &sge;
    wr.num_sge      = 1;
    wr.send_flags   = IBV_SEND_SIGNALED;
    // if (size <= MAX_INLINE_DATA)
    //    wr.send_flags |= IBV_SEND_INLINE;

    sge.addr   = mem.addr_;
    sge.length = size;
    sge.lkey   = mem.mr->lkey;

    if (!post_send(id->qp, &wr, &bad_wr))
        throw std::runtime_error("read_rdma_async: error post_send");

    return op_info;
}

/**
 * Method that fetches value from a remote address, then adds a value to it
 * before pushing it back to the remote store.
 */
template<class T>
void RDMAClient<T>::fetchadd_rdma_sync(struct rdma_cm_id *id,
        uint64_t remote_addr, uint64_t peer_rkey, uint64_t value) {
    LOG<INFO>("RDMAClient:: fetchadd_rdma_sync");
    auto op_info = fetchadd_rdma_async(id, remote_addr, peer_rkey, value);
    LOG<INFO>("RDMAClient:: waiting");

    // wait until operation is completed
    {
        TimerFunction tf("waiting semaphore", true);
        op_info->op_sem->wait();
    }

    delete op_info->op_sem;
    delete op_info;
}

/**
 * Method that fetches value from a remote address, then adds a value to it
 * before pushing it back to the remote store.
 */
template<class T>
typename RDMAClient<T>::RDMAOpInfo*
RDMAClient<T>::fetchadd_rdma_async(struct rdma_cm_id *id,
        uint64_t remote_addr, uint64_t peer_rkey, uint64_t /*value*/) {
#if __GNUC__ >= 7
    [[maybe_unused]]
#else
    __attribute__((unused))
#endif
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    struct ibv_send_wr wr, *bad_wr = nullptr;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));
    memset(&sge, 0, sizeof(sge));

    Lock* l = new SpinLock();
    l->wait();

    auto op_info = new RDMAClient::RDMAOpInfo(id, l);
    wr.wr_id = reinterpret_cast<uint64_t>(op_info);
    wr.opcode = IBV_WR_ATOMIC_FETCH_AND_ADD;
    wr.wr.atomic.remote_addr = remote_addr;
    wr.wr.atomic.rkey = peer_rkey;
    wr.wr.atomic.compare_add = peer_rkey;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    if (!post_send(id->qp, &wr, &bad_wr))
        throw std::runtime_error("fetchadd_rdma_async: error post_send");

    return op_info;
}

/**
 * Connects to the RDMA host at the given ip and port.
 * @param host the ip address of the host
 * @param port the port the host is listening on
 */
template<class T>
void RDMAClient<T>::connect_rdma_cm(const std::string& host,
                                 const std::string& port) {
    struct addrinfo *addr;
    struct rdma_conn_param cm_params;
    struct rdma_cm_event *event = nullptr;

    TimerFunction tf("connection end-to-end time", true);

    // use connection manager to resolve address
    TEST_NZ(getaddrinfo(host.c_str(), port.c_str(), nullptr, &addr));
    ec_ = rdma_create_event_channel();
    TEST_Z(ec_ = rdma_create_event_channel());
    TEST_NZ(rdma_create_id(ec_, &id_, nullptr, RDMA_PS_TCP));
    TEST_NZ(rdma_resolve_addr(id_, nullptr, addr->ai_addr, timeout_ms_));

    LOG<INFO>("Created rdma_cm_id: ",
        reinterpret_cast<uint64_t>(id_));

    freeaddrinfo(addr);

    id_->context = &con_ctx_;
    build_params(&cm_params);

    while (rdma_get_cm_event(ec_, &event) == 0) {
        struct rdma_cm_event event_copy;

        LOG<INFO>("New rdma_get_cm_event");

        memcpy(&event_copy, event, sizeof(*event));
        rdma_ack_cm_event(event);

        if (event_copy.event == RDMA_CM_EVENT_ADDR_RESOLVED) {
            // create connection and event loop
            build_connection(event_copy.id);

            LOG<INFO>("id: ",
                reinterpret_cast<uint64_t>(event_copy.id));

            setup_memory(&con_ctx_);
            TEST_NZ(rdma_resolve_route(event_copy.id, timeout_ms_));
            LOG<INFO>("resolved route");
        } else if (event_copy.event == RDMA_CM_EVENT_ROUTE_RESOLVED) {
            LOG<INFO>("Connecting (rdma_connect) to ", host, ":", port);
            TEST_NZ(rdma_connect(event_copy.id, &cm_params));
            LOG<INFO>("id: ",
                reinterpret_cast<uint64_t>(event_copy.id));
        } else if (event_copy.event == RDMA_CM_EVENT_ESTABLISHED) {
            LOG<INFO>("RDMA_CM_EVENT_ESTABLISHED");
            LOG<INFO>("id: ",
                reinterpret_cast<uint64_t>(event_copy.id));
            break;
        } else if (event_copy.event == RDMA_CM_EVENT_UNREACHABLE) {
            LOG<ERROR>("Server unreachable.");
            exit(-1);
        } else if (event_copy.event == RDMA_CM_EVENT_REJECTED) {
            LOG<ERROR>("Connection Rejected. Server may not be running.");
            exit(-1);
        } else if (event_copy.event == RDMA_CM_EVENT_CONNECT_ERROR) {
            LOG<ERROR>("Error establishing connection.");
            exit(-1);
        } else {
           LOG<ERROR>("Unexpected event type when connecting.");
           exit(-1);
        }
    }

    LOG<INFO>("Connection successful");
}

/**
  * A function that requests a given number of bytes be allocated
  * from the server.
  * @param size the number of bytes the client is requesting be allocated
  * @return AllocationRecord for the new AllocationRecord
  */
template<class T>
AllocationRecord RDMAClient<T>::allocate(uint64_t size) {
    LOG<INFO>("Allocating ",
        size, " bytes");

    // Create message using flatbuffers
    flatbuffers::FlatBufferBuilder builder(initial_buffer_size);
    auto data = message::BladeMessage::CreateAlloc(builder, size);
    auto alloc_msg = message::BladeMessage::CreateBladeMessage(builder,
                              message::BladeMessage::Data_Alloc, data.Union());
    builder.Finish(alloc_msg);

    int message_size = builder.GetSize();

    // Copy message contents into send buffer
    std::memcpy(con_ctx_.send_msg,
                builder.GetBufferPointer(),
                message_size);


    // post receive
    LOG<INFO>("Sending alloc msg size: ", message_size);
    send_receive_message_sync(id_, message_size);
    LOG<INFO>("send_receive_message_sync done: ", message_size);

    auto msg = message::BladeMessage::GetBladeMessage(con_ctx_.recv_msg);

    if (msg->data_as_AllocAck()->remote_addr() == 0) {
      // Throw error message
      LOG<ERROR>("Server threw exception when allocating memory.");
      throw cirrus::ServerMemoryErrorException("Server threw "
               "exception when allocating memory.");
    }

    AllocationRecord alloc(
                msg->data_as_AllocAck()->mr_id(),
                msg->data_as_AllocAck()->remote_addr(),
                msg->data_as_AllocAck()->peer_rkey());

    LOG<INFO>("Received allocation. mr_id: ", msg->data_as_AllocAck()->mr_id(),
            " remote_addr: " , msg->data_as_AllocAck()->remote_addr(),
            " peer_rkey: ", msg->data_as_AllocAck()->peer_rkey());

    if (msg->data_as_AllocAck()->remote_addr() == 0)
        throw std::runtime_error("Error with allocation");

    LOG<INFO>("Received allocation from Blade. remote_addr: ",
        msg->data_as_AllocAck()->remote_addr(),
        " mr_id: ", msg->data_as_AllocAck()->mr_id());
    return alloc;
}

/**
  * A function that requests a given allocation be freed in the remote store.
  * @param ar the allocation record that records the allocation to be freed.
  * @return success
  */
template<class T>
bool RDMAClient<T>::deallocate(const AllocationRecord& ar) {
    LOG<INFO>("Deallocating addr: ", ar.remote_addr);

    flatbuffers::FlatBufferBuilder builder(initial_buffer_size);
    auto data = message::BladeMessage::CreateDealloc(builder, ar.remote_addr);
    auto dealloc_msg = message::BladeMessage::CreateBladeMessage(builder,
                            message::BladeMessage::Data_Dealloc, data.Union());
    builder.Finish(dealloc_msg);
    int message_size = builder.GetSize();
    // Copy message into send buffer
    std::memcpy(con_ctx_.send_msg,
                builder.GetBufferPointer(),
                message_size);


    // post receive
    LOG<INFO>("Sending dealloc msg size: ", message_size);
    send_receive_message_sync(id_, message_size);
    LOG<INFO>("send_receive_message_sync done: ", message_size);

    auto msg = message::BladeMessage::GetBladeMessage(con_ctx_.recv_msg);

    if (msg->data_as_DeallocAck()->result() == 0)
        throw std::runtime_error("Error with deallocation");
    return true;
}

/**
  * Write synchronously.
  * A function that writes data to the remote store synchronously. Reads from
  * data and passes length bytes.
  * @param alloc_rec the AllocationRecord corresponding to an adequately sized
  * allocation.
  * @param offset the offset from data at which to start reading
  * @param length the number of bytes to send
  * @param mem pointer to RDMAMem struct for this write
  * @return write success
  * @see AllocationRecord
  */
template<class T>
bool RDMAClient<T>::rdma_write_sync(const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data,
        RDMAMem* mem) {
    LOG<INFO>("writing rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    if (length > SEND_MSG_SIZE)
        return false;

    if (mem) {
        {
            // TimerFunction tf("BladeClient::write_sync prepare", true);
            mem->addr_ = reinterpret_cast<uint64_t>(data);
            mem->prepare(con_ctx_.gen_ctx_);
        }

        write_rdma_sync(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey,
                *mem);

    } else {
        std::memcpy(con_ctx_.send_msg, data, length);
        write_rdma_sync(id_, length,
                alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
                default_send_mem_);
    }

    return true;
}

/**
  * Write asynchronously.
  * A function that writes data to the remote store asynchronously. Reads from
  * data and passes length bytes.
  * @param alloc_rec the AllocationRecord corresponding to an adequately sized
  * allocation.
  * @param offset the offset from data at which to start reading
  * @param length the number of bytes to send
  * @param mem pointer to RDMAMem struct for this write
  * @return std::shared_ptr<FutureBladeOp> a shared_ptr to a FutureBladeOp
  * that the caller can then extract a future from. Null if length >
  * the max sendable message size
  * @see AllocationRecord
  */
template<class T>
std::shared_ptr<typename RDMAClient<T>::FutureBladeOp>
RDMAClient<T>::rdma_write_async(
        const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data,
        RDMAMem* mem) {
    LOG<INFO>("writing rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    if (length > SEND_MSG_SIZE) {
        LOG<ERROR>("Message too large to send");
        return nullptr;
    }

    RDMAClient::RDMAOpInfo* op_info = nullptr;
    if (mem) {
        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->size_ = length;
        mem->prepare(con_ctx_.gen_ctx_);

        op_info = write_rdma_async(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey,
                *mem);
    } else {
        RDMAMem* mem = new RDMAMem(data, length);

        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->prepare(con_ctx_.gen_ctx_);

        op_info = write_rdma_async(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey,
                *mem);
    }

    return std::make_shared<RDMAClient::FutureBladeOp>(op_info);
}

/**
  * Read synchronously.
  * A function that reads data from the remote store synchronously. Reads
  * length bytes offset by offset bytes from data.
  * @param alloc_rec the AllocationRecord containing the necessary info
  * @param offset the offset from the remote_addr at which to start reading
  * @param length the number of bytes to read
  * @param data the location in local memory to write to
  * @param mem pointer to RDMAMem struct for this read
  * @return success if length is less than max receivable size
  */
template<class T>
bool RDMAClient<T>::rdma_read_sync(const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t length,
        void *data,
        RDMAMem* mem) {
    if (length > RECV_MSG_SIZE) {
        LOG<ERROR>("Message too long to receive.");
        return false;
    }

    LOG<INFO>("reading rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    if (mem) {
        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->size_ = length;
        mem->prepare(con_ctx_.gen_ctx_);

        read_rdma_sync(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey, *mem);
    } else {
        read_rdma_sync(id_, length,
                alloc_rec.remote_addr + offset,
                alloc_rec.peer_rkey, default_recv_mem_);

        {
            TimerFunction tf("Memcpy time", true);
            std::memcpy(data, con_ctx_.recv_msg, length);
        }
    }

    return true;
}

/**
  * Read asynchronously.
  * A function that reads data from the remote store asynchronously. Reads
  * length bytes offset by offset bytes from the remote address.
  * @param alloc_rec the AllocationRecord containing the necessary info
  * @param offset the offset from the remote_addr at which to start reading
  * @param length the number of bytes to read
  * @param data the location in local memory to write to
  * @param mem pointer to RDMAMem struct for this read
  * @return std::shared_ptr<FutureBladeOp> a shared_ptr to a FutureBladeOp
  * that the caller can then extract a future from. Null if length >
  * the max receivable message size
  */
template<class T>
std::shared_ptr<typename RDMAClient<T>::FutureBladeOp>
RDMAClient<T>::rdma_read_async(
        const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t length,
        void *data,
        RDMAMem* mem) {
    if (length > RECV_MSG_SIZE)
        return nullptr;

    LOG<INFO>("reading rdma",
        " length: ", length,
        " offset: ", offset,
        " remote_addr: ", alloc_rec.remote_addr,
        " rkey: ", alloc_rec.peer_rkey);

    RDMAClient::RDMAOpInfo* op_info;
    if (mem) {
        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->prepare(con_ctx_.gen_ctx_);

        op_info = read_rdma_async(id_, length,
                alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
                *mem,
                []() -> void {});
    } else {
        RDMAMem* mem = new RDMAMem(data, length);

        mem->addr_ = reinterpret_cast<uint64_t>(data);
        mem->prepare(con_ctx_.gen_ctx_);

        op_info = read_rdma_async(id_, length,
                alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
                *mem,
                [mem]() -> void { delete mem; });
    }

    return std::make_shared<RDMAClient::FutureBladeOp>(op_info);
}

/**
 * A wrapper for fetchadd_rdma_sync.
 */
template<class T>
bool RDMAClient<T>::fetchadd_sync(const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t value) {
    LOG<INFO>("fetchadd (sync) rdma",
            " offset: ", offset,
            " value: ", value);

    fetchadd_rdma_sync(id_,
            alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
            value);

    return true;
}

/**
 * A wrapper for fetchadd_rdma_async.
 */
template<class T>
std::shared_ptr<typename RDMAClient<T>::FutureBladeOp>
RDMAClient<T>::fetchadd_async(
        const AllocationRecord& alloc_rec,
        uint64_t offset,
        uint64_t value) {
    LOG<INFO>("fetchadd (sync) rdma",
            " offset: ", offset,
            " value: ", value);

    RDMAClient::RDMAOpInfo* op_info = fetchadd_rdma_async(id_,
            alloc_rec.remote_addr + offset, alloc_rec.peer_rkey,
            value);

    return std::make_shared<RDMAClient::FutureBladeOp>(op_info);
}


/**
  * Wait for operation to finish.
  * A function that calls the FutureBladeOp's operation's wait() method.
  * Waits until the operation is complete.
  * @see RDMAOpInfo
  * @see Lock
  */
template<class T>
void RDMAClient<T>::FutureBladeOp::wait() {
    op_info->op_sem->wait();
}

/**
  * Check operation status.
  * A function that calls the FutureBladeOp's operation's try_wait() method.
  * Returns instantly with the status of the operation.
  * @return true if operation is finished, false otherwise
  * @see RDMAOpInfo
  * @see Lock
  */
template<class T>
bool RDMAClient<T>::FutureBladeOp::try_wait() {
    return op_info->op_sem->trywait();
}


}  // namespace cirrus

#endif  // SRC_CLIENT_RDMACLIENT_H_
