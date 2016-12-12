/* Copyright 2016 Joao Carreira */

#include "src/server/RDMAServer.h"

#include <unistd.h>

#include <functional>
#include <iostream>
#include <cstring>
#include <algorithm>

#include "src/common/ThreadPinning.h"
#include "src/utils/logging.h"

namespace sirius {

GeneralContext RDMAServer::gen_ctx_;

RDMAServer::RDMAServer(int port, int timeout_ms) {
    port_ = port;
    timeout_ms_ = timeout_ms;
    id_ = nullptr;
    ec_ = nullptr;

    std::cout.setf(std::ios::unitbuf);
}

RDMAServer::~RDMAServer() {
    rdma_destroy_event_channel(ec_);

    if (gen_ctx_.pd)
        TEST_NZ(ibv_dealloc_pd(gen_ctx_.pd));
    if (gen_ctx_.cq)
        TEST_Z(ibv_destroy_cq(gen_ctx_.cq));
    if (id_) {
        rdma_destroy_qp(id_);
        rdma_destroy_id(id_);
    }
    // if (gen_ctx_.cq_poller_thread)
    //    delete gen_ctx_.cq_poller_thread;

    // clean all connection data
    std::for_each(conns_.begin(), conns_.end(),
            [](ConnectionContext* ptr) {
                delete ptr;
            });
}

void RDMAServer::init() {
    struct sockaddr_in addr;

    LOG<INFO>("RDMAServer::init");

    // get ready to connect
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);

    // create channel and listen for incoming connections
    TEST_Z(ec_ = rdma_create_event_channel());
    TEST_NZ(rdma_create_id(ec_, &id_, nullptr, RDMA_PS_TCP));

    LOG<INFO>("Created id: ", reinterpret_cast<uint64_t>(id_));

    TEST_NZ(rdma_bind_addr(id_, (struct sockaddr *)&addr));
    TEST_NZ(rdma_listen(id_, NUM_BACKLOG));
}

void RDMAServer::send_message(struct rdma_cm_id *id, uint64_t size) {
    LOG<INFO>("Sending message. size: ", size);

    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    struct ibv_send_wr wr, *bad_wr = nullptr;
    ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = reinterpret_cast<uint64_t>(id);
    wr.opcode = IBV_WR_SEND;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    if (size <= MAX_INLINE_DATA)
        wr.send_flags |= IBV_SEND_INLINE;

    sge.addr = reinterpret_cast<uint64_t>(ctx->send_msg);
    sge.length = size;
    sge.lkey = ctx->send_msg_mr->lkey;

    TEST_NZ(ibv_post_send(id->qp, &wr, &bad_wr));
    LOG<INFO>("Message sent");
}

void RDMAServer::post_msg_receive(struct rdma_cm_id *id) {
    auto ctx = reinterpret_cast<ConnectionContext*>(id->context);

    struct ibv_recv_wr wr, *bad_wr = nullptr;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = reinterpret_cast<uintptr_t>(id);
    wr.sg_list = &sge;
    wr.num_sge = 1;

    sge.addr = reinterpret_cast<uint64_t>(ctx->recv_msg);
    sge.length = RECV_MSG_SIZE;
    sge.lkey = ctx->recv_msg_mr->lkey;

    TEST_NZ(ibv_post_recv(id->qp, &wr, &bad_wr));

    LOG<INFO>("Posted receive in wq. id: ",
        reinterpret_cast<uint64_t>(id));
}

void RDMAServer::build_params(struct rdma_conn_param *params) {
    memset(params, 0, sizeof(*params));
    params->initiator_depth = params->responder_resources = 10;
    params->rnr_retry_count = 70;
    params->retry_count = 70;
}

/*
 * I tried to call this as soon as possible
 * 1) to avoid the call_once and 2) having to create
 * a memory pool only when the first client connects
 * but couldn't get around that
 */
void RDMAServer::build_gen_context(struct ibv_context *verbs) {
    // set ibv_context
    gen_ctx_.ctx = verbs;

    TEST_Z(gen_ctx_.pd = ibv_alloc_pd(gen_ctx_.ctx));
    TEST_Z(gen_ctx_.comp_channel = ibv_create_comp_channel(gen_ctx_.ctx));
    TEST_Z(gen_ctx_.cq = ibv_create_cq(gen_ctx_.ctx, 100,
                nullptr, gen_ctx_.comp_channel, 0));
    TEST_NZ(ibv_req_notify_cq(gen_ctx_.cq, 0));

    //// Test this
    // ibv_pd* pd;
    // ibv_mw* mw;
    //// TEST_Z(pd = ibv_alloc_pd(verbs));
    // TEST_Z(mw = ibv_alloc_mw(gen_ctx_.pd, IBV_MW_TYPE_1));

    LOG<INFO>("Creating polling thread");
    gen_ctx_.cq_poller_thread = new std::thread(&RDMAServer::poll_cq);

    ThreadPinning::pinThread(gen_ctx_.cq_poller_thread->native_handle(),
            2);  // random core
}

void* RDMAServer::poll_cq() {
    struct ibv_cq *cq;
    struct ibv_wc wc;
    void* cq_ctx;

    LOG<INFO>("Polling..");

    while (1) {
        TEST_NZ(ibv_get_cq_event(gen_ctx_.comp_channel, &cq, &cq_ctx));
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
            }
        }
    }
}

void RDMAServer::build_qp_attr(struct ibv_qp_init_attr *qp_attr) {
    memset(qp_attr, 0, sizeof(*qp_attr));

    // same completion queue for receive and send
    qp_attr->send_cq = gen_ctx_.cq;
    qp_attr->recv_cq = gen_ctx_.cq;

    // we use reliable IB
    qp_attr->qp_type = IBV_QPT_RC;

    // at most 100 outstanding send/recv work requests
    qp_attr->cap.max_send_wr = 100;
    qp_attr->cap.max_recv_wr = 100;

    // max number of scatter gather work requests
    qp_attr->cap.max_send_sge = 1;
    qp_attr->cap.max_recv_sge = 1;

    // Allow inline data
    qp_attr->cap.max_inline_data = MAX_INLINE_DATA;
}

// I think we should only call this once per execution
// we can serve multiple clients with the same pd, qp and cq
void RDMAServer::build_connection(struct rdma_cm_id *id) {
    // create all the boleirplate required to
    // receive connections (queue pair, protection domain,
    // completion queue)
    // build_context(id->verbs);
    std::call_once(gen_ctx_flag,
            &RDMAServer::build_gen_context, this, id->verbs);

    struct ibv_qp_init_attr qp_attr;
    // setup queue pair attributes
    build_qp_attr(&qp_attr);

    // create queue pair
    // FIX: maybe put this qp in the ConnectionContext
    // to avoid needing this id (?)
    TEST_NZ(rdma_create_qp(id, gen_ctx_.pd, &qp_attr));

    LOG<INFO>("max_inline_data: ",
        qp_attr.cap.max_inline_data);
}

// connection has been established by now
// 1. post a receive request
// 2. publicize memory region to client
void RDMAServer::handle_established(struct rdma_cm_id *id) {
    for (int i = 0; i < 4; ++i)
        post_msg_receive(id);
}

void RDMAServer::handle_disconnected(struct rdma_cm_id *id) {
    auto conn_ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    ibv_dereg_mr(conn_ctx->recv_msg_mr);
    ibv_dereg_mr(conn_ctx->send_msg_mr);
}

void RDMAServer::create_connection_context(struct rdma_cm_id *id) {
    auto con_ctx = new ConnectionContext;

    conns_.push_back(con_ctx);
    // con_ctx->context_id = ConnectionContext::generateContextId();

    // Tie server object to connection context to
    // be able to handle messages later on
    con_ctx->server = this;
    id->context = con_ctx;

    // Create memory regions to be able to receive/send RDMA messages
    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&con_ctx->recv_msg),
        static_cast<size_t>(sysconf(_SC_PAGESIZE)), RECV_MSG_SIZE));
    TEST_Z(con_ctx->recv_msg_mr = ibv_reg_mr(
        gen_ctx_.pd, con_ctx->recv_msg, RECV_MSG_SIZE,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));

    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&con_ctx->send_msg),
        static_cast<size_t>(sysconf(_SC_PAGESIZE)), SEND_MSG_SIZE));
    TEST_Z(con_ctx->send_msg_mr = ibv_reg_mr(
        gen_ctx_.pd, con_ctx->send_msg, SEND_MSG_SIZE,
        IBV_ACCESS_LOCAL_WRITE));
}

void RDMAServer::on_completion(struct ibv_wc *wc) {
    auto id = (struct rdma_cm_id *)(uintptr_t)wc->wr_id;
    auto con_ctx = reinterpret_cast<ConnectionContext*>(id->context);

    LOG<INFO>("Processing event..");

    if (wc->opcode == IBV_WC_RECV) {
        LOG<INFO>("Blade server received a message..");

        // post another receive WR
        post_msg_receive(id);
        LOG<INFO>("Posted new receive WR");

        //msg_handler f = &RDMAServer::process_message;
        std::invoke(&RDMAServer::process_message, con_ctx->server, id, con_ctx->recv_msg);
        //(con_ctx->server->*f)(id, con_ctx->recv_msg);

    } else if (wc->opcode == IBV_WC_SEND) {
        LOG<INFO>("Blade server sent a message..");
    } else {
        DIE("Dont know what is this opcode");
    }
}

/*
 * Should be called after init
 */
void RDMAServer::loop() {
    struct rdma_cm_event *event = nullptr;
    struct rdma_conn_param cm_params;

    build_params(&cm_params);

    LOG<INFO>("Running loop ");
    while (rdma_get_cm_event(ec_, &event) == 0) {
        struct rdma_cm_event event_copy;

        memcpy(&event_copy, event, sizeof(*event));
        rdma_ack_cm_event(event);

        LOG<INFO>("Checking event");

        switch (event_copy.event) {
            case RDMA_CM_EVENT_ADDR_RESOLVED:
                LOG<INFO>("RDMA_CM_EVENT_ADDR_RESOLVED");
                // address successfully resolved
                // happens when we try to connect
                build_connection(event_copy.id);

                create_connection_context(event_copy.id);

                TEST_NZ(rdma_resolve_route(event_copy.id, timeout_ms_));
                break;
            case RDMA_CM_EVENT_ROUTE_RESOLVED:
                LOG<INFO>("RDMA_CM_EVENT_ROUTE_RESOLVED");
                TEST_NZ(rdma_connect(event_copy.id, &cm_params));
                break;
            case RDMA_CM_EVENT_CONNECT_REQUEST:
                // Every new connection gets a new rdma_id

                LOG<INFO>("RDMA_CM_EVENT_CONNECT_REQUEST. id: ",
                        reinterpret_cast<uint64_t>(event_copy.id));
                build_connection(event_copy.id);
                create_connection_context(event_copy.id);
                TEST_NZ(rdma_accept(event_copy.id, &cm_params));
                break;
            case RDMA_CM_EVENT_ESTABLISHED:
                LOG<INFO>("RDMA_CM_EVENT_ESTABLISHED");
                handle_established(event_copy.id);
                handle_connection(event_copy.id);
                break;
            case RDMA_CM_EVENT_DISCONNECTED:
                LOG<INFO>("RDMA_CM_EVENT_DISCONNECTED");
                handle_disconnection(event_copy.id);
                rdma_destroy_qp(event_copy.id);
                handle_disconnected(event_copy.id);
                rdma_destroy_id(event_copy.id);
                break;
            case RDMA_CM_EVENT_ADDR_ERROR:
            case RDMA_CM_EVENT_ROUTE_ERROR:
            case RDMA_CM_EVENT_CONNECT_RESPONSE:
            case RDMA_CM_EVENT_CONNECT_ERROR:
            case RDMA_CM_EVENT_UNREACHABLE:
            case RDMA_CM_EVENT_REJECTED:
            case RDMA_CM_EVENT_DEVICE_REMOVAL:
            case RDMA_CM_EVENT_MULTICAST_JOIN:
            case RDMA_CM_EVENT_MULTICAST_ERROR:
            case RDMA_CM_EVENT_ADDR_CHANGE:
            case RDMA_CM_EVENT_TIMEWAIT_EXIT:
                LOG<INFO>("Unhandled event code: ", event_copy.event, ". We ignore and pray for the best");
                break;
            default:
                DIE("unknown event\n");
        }
    }
}

void RDMAServer::handle_connection(struct rdma_cm_id* id) {
    id = id;  // compiler warning
}

void RDMAServer::handle_disconnection(struct rdma_cm_id* id) {
    id = id;  // compiler warning
}

}  // namespace sirius
