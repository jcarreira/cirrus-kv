#ifndef SRC_CLIENT_TCPCLIENT_CLIENT_H_
#define SRC_CLIENT_TCPCLIENT_CLIENT_H_

#include <string>
#include <thread>
#include <future>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include "src/common/schemas/TCPBladeMessage_generated.h"

namespace cirrus {

using ObjectID = uint64_t;
using TxnID = uint64_t;

/**
  * A TCP based client that inherits from BladeClient.
  */
class TCPClient {
 public:
    void connect(std::string address, std::string port);
    bool write_sync(ObjectID id, void* data, uint64_t size);
    bool read_sync(ObjectID id, void* data, uint64_t size);
    // std::future<bool> write_sync(ObjectID id, void* data, uint64_t size);
    // std::future<bool> read_sync(ObjectID id, void* data, uint64_t size);
    bool remove(ObjectID id);
    void test();

 private:
    /**
      * A struct shared between futures and the receiver_thread. Used to
      * notify client of operation completeion, as well as to complete
      * transactions.
      */
    struct txn_info {
        std::mutex mutex; /**< mutex for the CV/result/mem */
        std::condition_variable cv; /**< CV to let future wait */
        bool result = false; /**< result of the transaction */
        void *mem_for_read; /**< memory that should be read to */
    }

    int sock; /**< fd of the socket used to communicate w/ remote store. */
    TxnID curr_txn_id = 0; /**< next txn_id to assign */

    /**
      * Map that allows receiver thread to map transactions to their
      * completion information.
      */
    std::map<TxnID, std::shared_ptr<struct txn_info>> txn_map;
    std::queue<std::shared_ptr<flatbuffers::FlatBufferBuilder>> send_queue;

    std::mutex txn_map_mutex;

    std::mutex send_queue_mutex;
    std::condition_variable send_queue_cv;

    std::thread receiver_thread;
    std::thread sender_thread;

    void process_received();
    void process_send();
};

}  // namespace cirrus

#endif  // SRC_CLIENT_TCPCLIENT_H_
