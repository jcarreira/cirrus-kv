/* Copyright 2016 Joao Carreira */

#ifndef _BLADE_MESSAGE_H_
#define _BLADE_MESSAGE_H_

#include <cstdint>

namespace sirius {

enum msg_type { 
    ALLOC, ALLOC_ACK,
    STATS, STATS_ACK,
    ALLOC_COALESCED, ALLOC_COALESCED_ACK,
    DEALLOC, DEALLOC_ACK,
    LOCK, LOCK_ACK,
    FLUSH, FLUSH_ACK,
    SUBSCRIBE, SUBSCRIBE_ACK,
    KEEP_ALIVE, KEEP_ALIVE_ACK
};

struct BladeMessage {
    msg_type type;
    union {
        // Allocation
        struct {
            uint64_t size;
        } alloc;
        struct {
            uint64_t mr_id;
            uint64_t remote_addr;
            uint64_t peer_rkey;
        } alloc_ack;
        // Deallocation
        struct {
            uint64_t addr;
        } dealloc;
        struct {
            char result;
        } dealloc_ack;
    } data;
};


struct BladeObjectStoreMessage {
    msg_type type;
    union {
        // Allocation
        struct {
            uint64_t size;
        } alloc;
        struct {
            uint64_t mr_id;
            uint64_t remote_addr;
            uint64_t peer_rkey;
        } alloc_ack;
        // Deallocation
        struct {
            uint64_t addr;
        } dealloc;
        struct {
            char result;
        } dealloc_ack;
        // Keep alive
        struct {
            uint64_t rand;
        } keep_alive;
        struct {
            uint64_t rand;
        } keep_alive_ack;
        // Subscribe
        struct {
            uint64_t oid;
            char addr[20]; // ip:port (UD)
        } sub;
        struct {
            uint64_t oid;
        } sub_ack;
        // Flush
        struct {
            uint64_t oid;
        } flush;
        struct {
        } flush_ack;
        //  Lock
        struct {
            uint64_t id; // lock id or bin
        } lock;
        struct {
            uint64_t id;
        } lock_ack;
    } data;
};

struct BladeMessageBig : public BladeMessage {
    char data[1024];
};

} // sirius

#endif // _BLADE_MESSAGE_H_

