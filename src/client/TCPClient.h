#ifndef SRC_CLIENT_TCPCLIENT_H_
#define SRC_CLIENT_TCPCLIENT_H_

#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <vector>
#include <algorithm>
#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <queue>
#include <utility>
#include <atomic>
#include <unordered_map>
#include "common/schemas/TCPBladeMessage_generated.h"
#include "libcuckoo/cuckoohash_map.hh"
#include "client/BladeClient.h"
#include "common/Exception.h"
#include "common/Serializer.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include <boost/lockfree/queue.hpp>

#define SEND_QUEUE_SIZE 10000

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

    bool write_sync(ObjectID id, const WriteUnit& w) override;
    std::pair<std::shared_ptr<const char>, unsigned int> read_sync(
        ObjectID oid) override;
    std::pair<std::shared_ptr<const char>, unsigned int> read_sync_bulk(
        const std::vector<ObjectID>& ids) override;


    ClientFuture write_async(ObjectID oid, const WriteUnit& w) override;
    ClientFuture read_async(ObjectID oid) override;
    ClientFuture read_async_bulk(std::vector<ObjectID> oids) override;

    bool remove(ObjectID id) override;

 private:
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
        std::shared_ptr<cirrus::SpinLock> sem;

        /** Pointer to shared ptr that points to any mem allocated for reads. */
        std::shared_ptr<std::shared_ptr<const char>> mem_for_read_ptr;

        /** Pointer to size of mem for read. */
        std::shared_ptr<uint64_t> mem_size;

        txn_info() {
            result = std::make_shared<bool>();
            result_available = std::make_shared<bool>();
            *result_available = false;
            sem = std::make_shared<cirrus::SpinLock>();
            error_code = std::make_shared<cirrus::ErrorCodes>();
            mem_for_read_ptr = std::make_shared<std::shared_ptr<const char>>();
            mem_size = std::make_shared<uint64_t>(0);
        }
    };

    /**
     * Custom deleter to allow for the deletion of a shared ptr to a
     * char to delete the vector that contains the char.
     */
    class read_op_deleter {
     public:
        /**
         * Constructor for the deleter.
         * @param buf pointer to the vector that contains the character.
         */
        explicit read_op_deleter(
            std::shared_ptr<std::vector<char>> buf) :
            buffer(buf) {}

        /**
         * Function that actually performs the deletion. Does not need to do
         * anything as going out of scope should eliminate the pointer to the
         * vector and delete the vector itself. ptr is a pointer to a character
         * that lives somewhere inside of the std::vector buf.
         */
        void operator()(const char * /* ptr */) {}

     private:
         /** Pointer to the vector that contains the data. */
         std::shared_ptr<std::vector<char>> buffer;
    };

    ssize_t send_all(int, const void*, size_t, int);
    ssize_t read_all(int sock, void* data, size_t len);

    ClientFuture enqueue_message(
                        flatbuffers::FlatBufferBuilder* builder,
                        const int txn_id);
    void process_received();
    void process_send();

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
    cuckoohash_map<TxnID, std::shared_ptr<struct txn_info>> txn_map;

    /**
     * Queue of FlatBufferBuilders that the sender_thread processes to send
     * messages to the server.
     */
    boost::lockfree::queue<flatbuffers::FlatBufferBuilder*,
        boost::lockfree::capacity<SEND_QUEUE_SIZE>> send_queue;

    /**
     * Queue of FlatBufferBuilders that are ready for reuse for writes.
     */
    std::queue<flatbuffers::FlatBufferBuilder*> reuse_queue;

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

    /**
     * Bool that indicates whether the client has already connected to a remote
     * store.
     */
    std::atomic<bool> has_connected = {false};
};

}  // namespace cirrus

#endif  // SRC_CLIENT_TCPCLIENT_H_
