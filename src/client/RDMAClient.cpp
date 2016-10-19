/* Copyright 2016 Joao Carreira */

#include "src/client/RDMAClient.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include <string>
#include <cstring>

#include "src/common/ThreadPinning.h"
#include "src/utils/utils.h"
#include "src/utils/TimerFunction.h"
#include "src/utils/easylogging++.h"

namespace sirius {

// Define static variables

// GeneralContext RDMAClient::gen_ctx_;

RDMAClient::RDMAClient(int timeout_ms) {
    id_ = NULL;
    ec_ = NULL;
    timeout_ms_ = timeout_ms;

    TEST_NZ(sem_init(&con_ctx.rdma_sem, 0, 0));
    TEST_NZ(sem_init(&con_ctx.send_sem, 0, 0));
    TEST_NZ(sem_init(&con_ctx.recv_sem, 0, 0));
}

RDMAClient::~RDMAClient() {
    // we can't free a running thread (?)
    // delete gen_ctx_.cq_poller_thread;
}

void RDMAClient::build_params(struct rdma_conn_param *params) {
    memset(params, 0, sizeof(*params));
    params->initiator_depth = params->responder_resources = 1;
    params->rnr_retry_count = 7; /* infinite retry */
}

void RDMAClient::setup_memory(ConnectionContext& ctx) {
    LOG(INFO) << "Setting up memory";
    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&ctx.recv_msg),
                sysconf(_SC_PAGESIZE),
                RECV_MSG_SIZE));

    {
        TimerFunction tf("Registering memory region", true);
        TEST_Z(con_ctx.recv_msg_mr =
                ibv_reg_mr(ctx.gen_ctx_.pd, con_ctx.recv_msg,
                    RECV_MSG_SIZE,
                    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));
    }


    TEST_NZ(posix_memalign(reinterpret_cast<void **>(&con_ctx.send_msg),
                sysconf(_SC_PAGESIZE),
                SEND_MSG_SIZE));
    TEST_Z(con_ctx.send_msg_mr = ibv_reg_mr(ctx.gen_ctx_.pd, con_ctx.send_msg,
                SEND_MSG_SIZE,
                IBV_ACCESS_LOCAL_WRITE |
                IBV_ACCESS_REMOTE_WRITE));
}

void RDMAClient::build_qp_attr(struct ibv_qp_init_attr *qp_attr,
        ConnectionContext* ctx) {
    memset(qp_attr, 0, sizeof(*qp_attr));

    qp_attr->send_cq = ctx->gen_ctx_.cq;
    qp_attr->recv_cq = ctx->gen_ctx_.cq;
    qp_attr->qp_type = IBV_QPT_RC;

    qp_attr->cap.max_send_wr = 10;
    qp_attr->cap.max_recv_wr = 10;
    qp_attr->cap.max_send_sge = 1;
    qp_attr->cap.max_recv_sge = 1;
}

int RDMAClient::post_receive(struct rdma_cm_id *id) {
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    LOG(INFO) << "Posting receive";

    struct ibv_recv_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = reinterpret_cast<uint64_t>(id);
    wr.sg_list = &sge;
    wr.num_sge = 1;

    sge.addr = reinterpret_cast<uint64_t>(ctx->recv_msg);
    sge.length = 1000;  // FIX
    sge.lkey = ctx->recv_msg_mr->lkey;

    return ibv_post_recv(id->qp, &wr, &bad_wr);
}

void RDMAClient::build_connection(struct rdma_cm_id *id) {
    struct ibv_qp_init_attr qp_attr;

    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    build_context(id->verbs, ctx);
    build_qp_attr(&qp_attr, ctx);

    // create the QP we are going to use to communicate with
    TEST_NZ(rdma_create_qp(id, ctx->gen_ctx_.pd, &qp_attr));
}

void RDMAClient::build_context(struct ibv_context *verbs,
        ConnectionContext* ctx) {
    // FIX: we only need one context?
    ctx->gen_ctx_.ctx = verbs;

    TEST_Z(ctx->gen_ctx_.pd = ibv_alloc_pd(ctx->gen_ctx_.ctx));
    TEST_Z(ctx->gen_ctx_.comp_channel =
            ibv_create_comp_channel(ctx->gen_ctx_.ctx));
    TEST_Z(ctx->gen_ctx_.cq = ibv_create_cq(ctx->gen_ctx_.ctx,
                10, NULL, ctx->gen_ctx_.comp_channel, 0));
    TEST_NZ(ibv_req_notify_cq(ctx->gen_ctx_.cq, 0));

    ctx->gen_ctx_.cq_poller_thread = new std::thread(poll_cq, ctx);

    ThreadPinning::pinThread(ctx->gen_ctx_.cq_poller_thread->native_handle(),
            1); // random core
}

void* RDMAClient::poll_cq(ConnectionContext* ctx) {
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

    return NULL;
}

void RDMAClient::on_completion(struct ibv_wc *wc) {
    struct rdma_cm_id *id = (struct rdma_cm_id *)(uintptr_t)(wc->wr_id);
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    LOG(INFO) << "wr_id: " << wc->wr_id
        << " opcode: " << wc->opcode
        << " byte_len: " << wc->byte_len;

    switch (wc->opcode) {
        case IBV_WC_RECV:
            sem_post(&ctx->recv_sem);
            // TEST_NZ(post_receive(id_));
            break;
        case IBV_WC_RDMA_READ:
        case IBV_WC_RDMA_WRITE:
            sem_post(&ctx->rdma_sem);
            break;
        case IBV_WC_SEND:
            sem_post(&ctx->send_sem);
            break;
        default:
            LOG(ERROR) << "Unknown opcode";
            exit(-1);
            break;
    }
}

void RDMAClient::send_message_sync(struct rdma_cm_id *id, uint64_t size) {
    ConnectionContext *con_ctx =
        reinterpret_cast<ConnectionContext*>(id->context);
    send_message(id, size);

    sem_wait(&con_ctx->recv_sem);
}

void RDMAClient::send_message(struct rdma_cm_id *id, uint64_t size) {
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = reinterpret_cast<uint64_t>(id);
    wr.opcode = IBV_WR_SEND;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    sge.addr = reinterpret_cast<uint64_t>(ctx->send_msg);
    sge.length = size;
    sge.lkey = ctx->send_msg_mr->lkey;

    if (ibv_post_send(id->qp, &wr, &bad_wr)) {
        LOG(ERROR) << "Error post_send."
            << " errno: " << errno;
    }
}

void RDMAClient::write_rdma_sync(struct rdma_cm_id *id, uint64_t size,
        uint64_t remote_addr, uint64_t peer_rkey) {
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    write_rdma(id, size, remote_addr, peer_rkey);

    // wait until operation is completed
    sem_wait(&ctx->rdma_sem);
}

void RDMAClient::write_rdma(struct rdma_cm_id *id, uint64_t size,
        uint64_t remote_addr, uint64_t peer_rkey) {
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));
    memset(&sge, 0, sizeof(sge));

    wr.wr_id = reinterpret_cast<uint64_t>(id);
    wr.opcode = IBV_WR_RDMA_WRITE;
    wr.wr.rdma.remote_addr = remote_addr;
    wr.wr.rdma.rkey = peer_rkey;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    sge.addr = reinterpret_cast<uint64_t>(ctx->send_msg);
    sge.length = size;
    sge.lkey = ctx->send_msg_mr->lkey;

    if (ibv_post_send(id->qp, &wr, &bad_wr)) {
        LOG(ERROR) << "Error post_send."
            << " errno: " << errno;
    }
}

void RDMAClient::read_rdma(struct rdma_cm_id *id, uint64_t size,
        uint64_t remote_addr, uint64_t peer_rkey) {
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));
    memset(&sge, 0, sizeof(sge));

    wr.wr_id = reinterpret_cast<uint64_t>(id);
    wr.opcode = IBV_WR_RDMA_READ;
    wr.wr.rdma.remote_addr = remote_addr;
    wr.wr.rdma.rkey = peer_rkey;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    sge.addr = reinterpret_cast<uint64_t>(ctx->recv_msg);
    sge.length = size;
    sge.lkey = ctx->recv_msg_mr->lkey;

    if (ibv_post_send(id->qp, &wr, &bad_wr)) {
        LOG(ERROR) << "Error post_send."
            << " errno: " << errno;
    }
}

void RDMAClient::read_rdma_sync(struct rdma_cm_id *id, uint64_t size,
        uint64_t remote_addr, uint64_t peer_rkey) {
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    read_rdma(id, size, remote_addr, peer_rkey);

    // wait until operation is completed
    sem_wait(&ctx->rdma_sem);
}

void RDMAClient::connect(std::string host, std::string port) {
    struct addrinfo *addr;
    struct rdma_conn_param cm_params;
    struct rdma_cm_event *event = NULL;

    // use connection manager to resolve address
    TEST_NZ(getaddrinfo(host.c_str(), port.c_str(), NULL, &addr));
    TEST_Z(ec_ = rdma_create_event_channel());
    TEST_NZ(rdma_create_id(ec_, &id_, NULL, RDMA_PS_TCP));
    TEST_NZ(rdma_resolve_addr(id_, NULL, addr->ai_addr, timeout_ms_));

    LOG(INFO) << "Created id: " 
        << reinterpret_cast<uint64_t>(id_);

    freeaddrinfo(addr);

    id_->context = &con_ctx;
    build_params(&cm_params);

    while (rdma_get_cm_event(ec_, &event) == 0) {
        struct rdma_cm_event event_copy;
            
        LOG(INFO) << "New event";

        memcpy(&event_copy, event, sizeof(*event));
        rdma_ack_cm_event(event);

        if (event_copy.event == RDMA_CM_EVENT_ADDR_RESOLVED) {
            // create connection and event loop
            build_connection(event_copy.id);

            LOG(INFO) << "id: " 
                << reinterpret_cast<uint64_t>(event_copy.id);

            setup_memory(con_ctx);
            TEST_NZ(post_receive(id_));
            TEST_NZ(rdma_resolve_route(event_copy.id, timeout_ms_));
            LOG(INFO) << "resolved route";
        } else if (event_copy.event == RDMA_CM_EVENT_ROUTE_RESOLVED) {
            LOG(INFO) << "Connecting (rdma_connect)";
            TEST_NZ(rdma_connect(event_copy.id, &cm_params));
            LOG(INFO) << "id: " 
                << reinterpret_cast<uint64_t>(event_copy.id);
        } else if (event_copy.event == RDMA_CM_EVENT_ESTABLISHED) {
            LOG(INFO) << "RDMA_CM_EVENT_ESTABLISHED";
            LOG(INFO) << "id: " 
                << reinterpret_cast<uint64_t>(event_copy.id);
            break;
        }
    }
            
    LOG(INFO) << "Connection successful";
}

} // sirius
