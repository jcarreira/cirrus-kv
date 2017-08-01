#ifndef SRC_CLIENT_TCPCLIENT_H_
#define SRC_CLIENT_TCPCLIENT_H_

#include <string>
#include <thread>
#include <queue>
#include <map>
#include <atomic>
#include "common/schemas/TCPBladeMessage_generated.h"
#include "client/BladeClient.h"
#include "common/Future.h"
#include "common/Exception.h"

namespace cirrus {

using ObjectID = uint64_t;
using TxnID = uint64_t;

/**
  * A TCP based client that inherits from BladeClient.
  */
class TCPClient : public BladeClient {
 public:
    ~TCPClient() override;
    void connect(const std::string& address,
        const std::string& port) override;

    bool write_sync(ObjectID oid, const void* data, uint64_t size) override;
    bool read_sync(ObjectID oid, void* data, uint64_t size) override;

    cirrus::Future write_async(ObjectID oid, const void* data,
                                       uint64_t size);
    cirrus::Future read_async(ObjectID oid, void* data, uint64_t size);

    bool remove(ObjectID id) override;

 private:
    ssize_t send_all(int, const void*, size_t, int);
    cirrus::Future enqueue_message(
                        std::shared_ptr<flatbuffers::FlatBufferBuilder> builder,
                        const int txn_id,
                        void *ptr = nullptr);
    void process_received();
    void process_send();

    /**
      * A struct shared between futures and the receiver_thread. Used to
      * notify client of operation completeion, as well as to complete
      * transactions.
      */
    struct txn_info {
        /** result of the transaction */
        std::shared_ptr<bool> result;
        /** Boolean indicating whether transaction is complete */
        std::shared_ptr<bool> result_available;
        /** Error code if any were thrown on the server. */
        std::shared_ptr<cirrus::ErrorCodes> error_code;
        /** Semaphore for the transaction. */
        std::shared_ptr<cirrus::PosixSemaphore> sem;

        void *mem_for_read;  /**< memory that should be read to */

        txn_info() {
            result = std::make_shared<bool>();
            result_available = std::make_shared<bool>();
            *result_available = false;
            sem = std::make_shared<cirrus::PosixSemaphore>();
            error_code = std::make_shared<cirrus::ErrorCodes>();
        }
    };
    /** fd of the socket used to communicate w/ remote store */
    int sock = 0;
    /** Next txn_id to assign to a txn_info. Used as a unique identifier. */
    std::atomic<std::uint64_t> curr_txn_id = {0};

    /**
      * Map that allows receiver thread to map transactions to their
      * completion information. When a message is added to the send queue,
      * a struct txn_info is created and added to this map. This struct 
      * allows the receiver thread to place information regarding completion
      * as well as data in a location that is accessible to the future
      * corresponding to the transaction.
      */
    std::map<TxnID, std::shared_ptr<struct txn_info>> txn_map;
    /**
     * Queue of FlatBufferBuilders that the sender_thread processes to send
     * messages to the server.
     */
    std::queue<std::shared_ptr<flatbuffers::FlatBufferBuilder>> send_queue;

    /** Lock on the txn_map. */
    cirrus::SpinLock map_lock;
    /** Lock on the send_queue. */
    cirrus::SpinLock queue_lock;
    /** Semaphore for the send_queue. */
    cirrus::PosixSemaphore queue_semaphore;
    /** Thread that runs the receiving loop. */
    std::thread* receiver_thread;
    /** Thread that runs the sending loop. */
    std::thread* sender_thread;

    /**
     * Bool that the process_send and process_received threads check.
     * If it is true, they exit their loops so that they may
     * finish execution and join may be called on them. Set to true
     * in the class destructor.
     */
    bool terminate_threads = false;
};

}  // namespace cirrus

#endif  // SRC_CLIENT_TCPCLIENT_H_
