#ifndef SRC_SERVER_TCPSERVER_H_
#define SRC_SERVER_TCPSERVER_H_

#include <poll.h>
#include <vector>
#include <map>
#include <atomic>
#include <queue>
#include <thread>
#include <functional>
#include "server/Server.h"
#include "common/Synchronization.h"
#include "libcuckoo/cuckoohash_map.hh"
#include "common/schemas/TCPBladeMessage_generated.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
  * This class serves as a remote store that allows connection from
  * clients over TCP.
  * Clients can make requests for reading and writing data.
  */
class TCPServer : public Server {
 public:
    TCPServer(int port, uint64_t pool_size_, uint64_t num_threads = 1);
    ~TCPServer() = default;

    void init();

    void loop();

 private:
    bool process(int sock);

    ssize_t send_all(int, const void*, size_t, int);
    ssize_t read_all(int sock, void*, size_t len);
    bool read_from_client(std::vector<char>&, int, uint64_t&);

    bool testRemove(const struct pollfd& x);

    void wait_to_process();

    bool remove(ObjectID oid);

    void process_read(flatbuffers::FlatBufferBuilder *builder,
        const cirrus::message::TCPBladeMessage::TCPBladeMessage *msg);

    void process_write(flatbuffers::FlatBufferBuilder *builder,
        const cirrus::message::TCPBladeMessage::TCPBladeMessage *msg);

    void process_remove(flatbuffers::FlatBufferBuilder *builder,
        const cirrus::message::TCPBladeMessage::TCPBladeMessage *msg);

    bool send_ack(flatbuffers::FlatBufferBuilder *builder, int sock);
    void remove_unused_entries();
    void accept_incoming_connection();

    /** The port that the server is listening on. */
    int port_;
    /** The fd for the socket the server listens for incoming requests on. */
    int server_sock_ = 0;

    /** Map between objectid and data. Used to store all data. */
    cuckoohash_map<ObjectID, std::vector<int8_t>> store;

    /** Lock on the process_queue. */
    cirrus::SpinLock queue_lock;
    /**
     * Lock to prevent threads from modifying fds while poll call is underway.
     */
    cirrus::SpinLock pollfds_lock;
    /** Semaphore to notify processing threads of new data. */
    cirrus::PosixSemaphore queue_semaphore;
    /** Queue of references to pollfds to process. */
    std::queue<std::reference_wrapper<struct pollfd>> process_queue;
    /** Vector of threads. May be useful later. */
    std::vector<std::unique_ptr<std::thread>> threads_vector;

    /** Number of threads that are currently waiting to process. */
    std::atomic<std::uint64_t> waiting_threads = {0};
    /** Number of processing threads. */
    uint64_t num_threads;

    /** Maximum number of bytes that can be stored in the pool. */
    uint64_t pool_size;

    /**
     * Current number of bytes stored.
     * Even though this value is accessed atomically, it is possible for the
     * server to go over capacity if multiple processes each check the size
     * simultaneously, all see that it is below the maximum, and then proceed
     * to write.
     */
    std::atomic<std::uint64_t> curr_size = {0};

    /** Current max number of sockets open at once. */
    uint64_t max_fds = 100;

    /**
     * Index that the next socket accepted should have in the
     * array of struct pollfds.
     */
    uint64_t curr_index = 0;
    /**
     * How long the client will wait during a call to poll() before
     * timing out (in ms).
     */
    int timeout = 60 * 1000 * 3;
    /**
     * Vector that serves as a wrapper for a c style array containing
     * struct pollfd objects. Used for calls to poll(). New structs are
     * always inserted to the end of a continus range, indicated by curr_index.
     * When the vector reaches capacity, unused structs are removed and the
     * remaining items shifted.
     */
    std::vector<struct pollfd> fds = std::vector<struct pollfd>(max_fds);
};

}  // namespace cirrus

#endif  // SRC_SERVER_TCPSERVER_H_
