/* Copyright 2016 Joao Carreira */

#include "src/server/ResourceAllocator.h"
#include "src/authentication/DumbAuthenticator.h"
#include "src/authentication/ApplicationKey.h"
#include "src/authentication/AuthenticationToken.h"
#include "src/common/AllocatorMessageGenerator.h"

#include "src/utils/easylogging++.h"

/*
 * Users authenticate in the ResourceManager and get a token
 * this token is an encrypted structure with application ID
 * This token can be used to communicate with the datacenter controllers
 * (e.g., allocate resources)
 */

namespace sirius {

ResourceAllocator::ResourceAllocator(int port,
        int timeout_ms) :
    RDMAServer(port, timeout_ms),
    authenticator_(new DumbAuthenticator()) {
}

ResourceAllocator::~ResourceAllocator() {
}

void ResourceAllocator::init() {
    RDMAServer::init();
}

void ResourceAllocator::send_auth_token(rdma_cm_id* id,
        const AllocatorMessage& msg) {
    AuthenticationToken token =
        AuthenticationToken::create_default_allow_token();  // allow
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    AllocatorMessageGenerator::auth_ack1_msg(
            ctx->send_msg,
            token);
    send_message(id, sizeof(AllocatorMessage));
}

void ResourceAllocator::send_auth_refusal(rdma_cm_id* id,
        const AllocatorMessage& msg) {
    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    AuthenticationToken token =
        AuthenticationToken::create_default_deny_token();  // deny

    AllocatorMessageGenerator::auth_ack1_msg(
            ctx->send_msg,
            token);
    send_message(id, sizeof(AllocatorMessage));
}

void ResourceAllocator::process_message(rdma_cm_id* id, void* message) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(message);

    LOG(INFO) << "ResourceAllocator Received message";

    ConnectionContext *ctx =
        reinterpret_cast<ConnectionContext*>(id->context);

    switch (msg->type) {
        case AUTH1:
            LOG(INFO) << "ResourceAllocator AUTH1";
            // we get the application public key
            // we could also get an ID here
            // ApplicationKey* ak = &msg->data.auth.application_key;
            // AppId app_id = msg->data.auth.app_id;

            if (authenticator_->allowApplication(msg->data.auth1.app_id)) {
                send_auth_token(id, *msg);
            } else {
                send_auth_refusal(id, *msg);
            }

            break;
        default:
            LOG(ERROR) << "Unknown message";
            exit(-1);
            break;
    }
}

}  // namespace sirius
