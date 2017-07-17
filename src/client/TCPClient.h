#ifndef SRC_CLIENT_TCPCLIENT_H_
#define SRC_CLIENT_TCPCLIENT_H_

#include <string>
#include <thread>
#include <queue>
#include <map>
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
    virtual ~TCPClient();
    void connect(const std::string& address,
        const std::string& port) override;

    bool write_sync(ObjectID oid, const void* data, uint64_t size) override;
    bool read_sync(ObjectID oid, void* data, uint64_t size) override;

    virtual cirrus::Future write_async(ObjectID oid, const void* data,
                                       uint64_t size);
    virtual cirrus::Future read_async(ObjectID oid, void* data, uint64_t size);

    bool remove(ObjectID id) override;

 private:
    ssize_t send_all(int, const void*, size_t, int);
    cirrus::Future enqueue_message(
                        std::shared_ptr<flatbuffers::FlatBufferBuilder> builder,
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

    int sock = 0;  /**< fd of the socket used to communicate w/ remote store */
    TxnID curr_txn_id = 0;  /**< next txn_id to assign */

    /**
      * Map that allows receiver thread to map transactions to their
      * completion information.
      */
    std::map<TxnID, std::shared_ptr<struct txn_info>> txn_map;
    /**
     * Queue of FlatBufferBuilders that the sender_thread processes to send
     * messages to the server.
     */
    std::queue<std::shared_ptr<flatbuffers::FlatBufferBuilder>> send_queue;

    /**
     * Queue of FlatBufferBuilders that are ready for reuse for writes.
     */
    std::queue<std::shared_ptr<flatbuffers::FlatBufferBuilder>> reuse_queue;

    /** Max number of flatbuffer builders in reuse_queue. */
    const unsigned int reuse_max = 5;


    /** Lock on the txn_map. */
    cirrus::SpinLock map_lock;
    /** Lock on the send_queue. */
    cirrus::SpinLock queue_lock;
    /** Lock on the reuse_queue. */
    cirrus::SpinLock reuse_lock;
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
