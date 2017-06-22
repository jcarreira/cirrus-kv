#ifndef SRC_CLIENT_TCPCLIENT_H_
#define SRC_CLIENT_TCPCLIENT_H_

#include <string>
#include <thread>
#include <future>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include "common/schemas/TCPBladeMessage_generated.h"
#include "client/BladeClient.h"
#include "common/Future.h"

namespace cirrus {

using ObjectID = uint64_t;
using TxnID = uint64_t;

/**
  * A TCP based client that inherits from BladeClient.
  */
class TCPClient : public BladeClient {
 public:
    void connect(std::string address, std::string port) override;
    bool write_sync(ObjectID oid, void* data, uint64_t size) override;
    bool read_sync(ObjectID oid, void* data, uint64_t size) override;
    cirrus::Future write_async(ObjectID oid, void* data,
                               uint64_t size) override;
    cirrus::Future read_async(ObjectID oid, void* data, uint64_t size) override;
    bool remove(ObjectID id) override;

 private:
    /**
      * A struct shared between futures and the receiver_thread. Used to
      * notify client of operation completeion, as well as to complete
      * transactions.
      */
    struct txn_info {
        std::shared_ptr<bool> result; /**< result of the transaction */

        /** Semaphore for the transaction. */
        std::shared_ptr<cirrus::PosixSemaphore> sem;

        void *mem_for_read; /**< memory that should be read to */

        txn_info() {
            result = std::make_shared<bool>();
            sem = std::make_shared<cirrus::PosixSemaphore>();
        }
    };

    int sock; /**< fd of the socket used to communicate w/ remote store. */
    TxnID curr_txn_id = 0; /**< next txn_id to assign */

    /**
      * Map that allows receiver thread to map transactions to their
      * completion information.
      */
    std::map<TxnID, std::shared_ptr<struct txn_info>> txn_map;
    std::queue<std::shared_ptr<flatbuffers::FlatBufferBuilder>> send_queue;

    cirrus::SpinLock map_lock;
    cirrus::SpinLock queue_lock;
    std::thread receiver_thread;
    std::thread sender_thread;

    cirrus::Future enqueue_message(std::shared_ptr<flatbuffers::FlatBufferBuilder> builder,
		    void *ptr = nullptr);
    void process_received();
    void process_send();
};

}  // namespace cirrus

#endif  // SRC_CLIENT_TCPCLIENT_H_
