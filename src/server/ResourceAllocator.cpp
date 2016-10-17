/* Copyright 2016 Joao Carreira */

#include "src/server/ResourceAllocator.h"
#include "AllocatorMessage.h"

#include "src/utils/easylogging++.h"

namespace sirius {

ResourceAllocator::ResourceAllocator(int port, int timeout_ms)
    : RDMAServer(port, timeout_ms) {
}


ResourceAllocator::~ResourceAllocator() {
}

void ResourceAllocator::process_message(rdma_cm_id* id, void* message) {
    AllocatorMessage* msg =
        reinterpret_cast<AllocatorMessage*>(message);

    LOG(INFO) << "Received message";
}

} // sirius
