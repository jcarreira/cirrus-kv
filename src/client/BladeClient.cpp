/* Copyright 2016 Joao Carreira */

#include "src/client/BladeClient.h"
#include <unistd.h>
#include "src/common/BladeMessageGenerator.h"
#include "src/common/BladeMessage.h"
#include "src/utils/utils.h"
#include "src/utils/easylogging++.h"

namespace sirius {

BladeClient::BladeClient(int timeout_ms)
    : RDMAClient(timeout_ms) {
}

BladeClient::~BladeClient() {
}

AllocRec BladeClient::allocate(uint64_t size) {
    LOG(INFO) << "Allocating " << size << " bytes";

    BladeMessageGenerator::alloc_msg(con_ctx.send_msg,
            size);

    // post receive
    TEST_NZ(post_receive(id_));
    LOG(INFO) << "Sending alloc msg size: " << sizeof(BladeMessage);
    send_message_sync(id_, sizeof(BladeMessage));

    BladeMessage* msg =
        reinterpret_cast<BladeMessage*>(con_ctx.recv_msg);

    AllocRec alloc(new AllocationRecord(
              msg->data.alloc_ack.mr_id,
                msg->data.alloc_ack.remote_addr,
                msg->data.alloc_ack.peer_rkey));

    LOG(INFO) << "Received allocation from Blade. remote_addr: "
        << msg->data.alloc_ack.remote_addr
        << " mr_id: " << msg->data.alloc_ack.mr_id;
    return alloc;
}

// write to memory region
bool BladeClient::write(AllocRec alloc_rec,
        uint64_t offset,
        uint64_t length,
        const void* data) {
    std::memcpy(con_ctx.send_msg, data, length);

    LOG(INFO) << "writing rdma"
        << " length: " << length
        << " offset: " << offset
        << " remote_addr: " << alloc_rec->remote_addr
        << " rkey: " << alloc_rec->peer_rkey;

    write_rdma_sync(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey);

    return true;
}

// read from memory region
// given offset and length
// copy to data
bool BladeClient::read(AllocRec alloc_rec,
        uint64_t offset,
        uint64_t length,
        void *data) {
    LOG(INFO) << "read op";

    LOG(INFO) << "reading rdma"
        << " length: " << length
        << " offset: " << offset
        << " remote_addr: " << alloc_rec->remote_addr
        << " rkey: " << alloc_rec->peer_rkey;

    read_rdma_sync(id_, length,
            alloc_rec->remote_addr + offset, alloc_rec->peer_rkey);

    std::memcpy(data, con_ctx.recv_msg, length);

    return true;
}

}
