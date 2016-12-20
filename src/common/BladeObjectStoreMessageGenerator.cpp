/* Copyright 2016 Joao Carreira */

#include "src/common/BladeObjectStoreMessageGenerator.h"

namespace sirius {

void BladeObjectStoreMessageGenerator::subscribe_msg(
        void *data, uint64_t id) {
    BladeObjectStoreMessage* msg =
        reinterpret_cast<BladeObjectStoreMessage*>(data);

    msg->type = SUBSCRIBE;
    msg->data.sub.oid = id;
}

void BladeObjectStoreMessageGenerator::subscribe_ack_msg(
        void *data, uint64_t id) {
    BladeObjectStoreMessage* msg =
        reinterpret_cast<BladeObjectStoreMessage*>(data);

    msg->type = SUBSCRIBE_ACK;
    msg->data.sub_ack.oid = id;
}

void BladeObjectStoreMessageGenerator::lock_msg(
        void *data, uint64_t id) {
    BladeObjectStoreMessage* msg =
        reinterpret_cast<BladeObjectStoreMessage*>(data);

    msg->type = LOCK;
    msg->data.lock.id = id;
}

void BladeObjectStoreMessageGenerator::lock_ack_msg(
        void *data, uint64_t id) {
    BladeObjectStoreMessage* msg =
        reinterpret_cast<BladeObjectStoreMessage*>(data);

    msg->type = LOCK_ACK;
    msg->data.lock_ack.id = id;
}

void BladeObjectStoreMessageGenerator::keep_alive_msg(
        void *data, uint64_t tok) {
    BladeObjectStoreMessage* msg =
        reinterpret_cast<BladeObjectStoreMessage*>(data);

    msg->type = KEEP_ALIVE;
    msg->data.keep_alive.rand = tok;
}

void BladeObjectStoreMessageGenerator::keep_alive_ack_msg(
        void *data, uint64_t tok) {
    BladeObjectStoreMessage* msg =
        reinterpret_cast<BladeObjectStoreMessage*>(data);

    msg->type = KEEP_ALIVE_ACK;
    msg->data.keep_alive.rand = tok;
}

void BladeObjectStoreMessageGenerator::flush_msg(
        void *data, uint64_t oid) {
    BladeObjectStoreMessage* msg =
        reinterpret_cast<BladeObjectStoreMessage*>(data);

    msg->type = FLUSH;
    msg->data.flush.oid = oid;
}

void BladeObjectStoreMessageGenerator::flush_ack_msg(
        void *data, uint64_t) {
    BladeObjectStoreMessage* msg =
        reinterpret_cast<BladeObjectStoreMessage*>(data);

    msg->type = FLUSH_ACK;
}

} // namespace sirius
