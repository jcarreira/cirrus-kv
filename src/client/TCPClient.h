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
#include <map>
#include <atomic>
#include "common/schemas/TCPBladeMessage_generated.h"
#include "client/BladeClient.h"
#include "common/Exception.h"
#include "common/Serializer.h"
#include "utils/logging.h"
#include "utils/utils.h"

namespace cirrus {

using ObjectID = uint64_t;
using TxnID = uint64_t;

static const int initial_buffer_size = 50;

/**
  * A TCP based client that inherits from BladeClient.
  */
class TCPClient : public BladeClient {
 public:
    ~TCPClient() override;
    void connect(const std::string& address,
        const std::string& port) override;

    bool write_sync(ObjectID id, const WriteUnit& w) override;
    bool read_sync(ObjectID oid, void* data, uint64_t size) override;


    cirrus::BladeClient::ClientFuture write_async(ObjectID oid,
                            const WriteUnit& w) override;
    cirrus::BladeClient::ClientFuture read_async(ObjectID oid,
                            void* data, uint64_t size) override;

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

    ssize_t send_all(int, const void*, size_t, int);
    cirrus::BladeClient::ClientFuture enqueue_message(
                        std::shared_ptr<flatbuffers::FlatBufferBuilder> builder,
                        const int txn_id,
                        void *ptr = nullptr);
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

    /**
     * Bool that indicates whether the client has already connected to a remote
     * store.
     */
    std::atomic<bool> has_connected = {false};
};

/**
 * Destructor method for the TCPClient. Terminates the receiver and sender
 * threads so that the program will exit gracefully.
 */
TCPClient::~TCPClient() {
    terminate_threads = true;

    // We need to destroy threads if they are active
    // alternatively (best option) we make the threads non-blocking
    if (receiver_thread) {
        LOG<INFO>("Terminating receiver thread");
        auto rhandle = receiver_thread->native_handle();
        pthread_cancel(rhandle);
        receiver_thread->join();
        delete receiver_thread;
    }

    if (sender_thread) {
        LOG<INFO>("Terminating sender thread");
        queue_semaphore.signal();  // unblock sender thread
        sender_thread->join();
        delete sender_thread;
    }
}

/**
  * Connects the client to the remote server. Opens a socket and attempts to
  * connect on the given address and port.
  * @param address the ipv4 address of the server, represented as a string
  * @param port_string the port to connect to, represented as a string
  */
void TCPClient::connect(const std::string& address,
                        const std::string& port_string) {
    // Create socket
    LOG<INFO>("Creating socket");
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw cirrus::ConnectionException("Error when creating socket.");
    }
    LOG<INFO>("Setting options");
    int opt = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt))) {
        throw cirrus::ConnectionException("Error setting socket options.");
    }

    struct sockaddr_in serv_addr;

    // Set the type of address being used, assuming ip v4
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, address.c_str(), &serv_addr.sin_addr);
    // Convert port from string to int
    int port = stoi(port_string, nullptr);

    // Save the port in the info
    serv_addr.sin_port = htons(port);

    LOG<INFO>("Connecting to server");
    // Connect to the server
    if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        throw cirrus::ConnectionException("Client could "
                                          "not connect to server.");
    }
    LOG<INFO>("Launching threads");
    receiver_thread = new std::thread(&TCPClient::process_received, this);
    sender_thread   = new std::thread(&TCPClient::process_send, this);
}

/**
  * Asynchronously writes an object to remote storage under id.
  * @param id the id of the object the user wishes to write to remote memory.
  * @param data a pointer to the buffer where the serialized object should
  * be read read from.
  * @param size the size of the serialized object being read from
  * local memory.
  * @return A Future that contains information about the status of the
  * operation.
  */
cirrus::BladeClient::ClientFuture TCPClient::write_async(ObjectID oid,
    const WriteUnit& w) {
    // Create flatbuffer builder if none to reuse;

    // Add the builder to the queue if it is of the right type (a write)
    // And if not over capacity
    std::shared_ptr<flatbuffers::FlatBufferBuilder> builder;
    // Size of serialized object, ASSUMED TO BE CONSTANT
    // TODO(Tyler): Change this! Allow variable sizes!
    uint64_t size = w.size();
    reuse_lock.wait();
    if (!reuse_queue.empty()) {
        builder = reuse_queue.front();
        reuse_queue.pop();
        reuse_lock.signal();
    } else {
        reuse_lock.signal();
        builder = std::make_shared<flatbuffers::FlatBufferBuilder>(size + 64);
    }


    // Create and send write request
    // Pointer to the vector inside of the flatbuffer to write to
    int8_t *mem;
    auto data_fb_vector = builder->CreateUninitializedVector(size, &mem);
    w.serialize(mem);
    auto msg_contents = message::TCPBladeMessage::CreateWrite(*builder,
                                                              oid,
                                                              data_fb_vector);
    const int txn_id = curr_txn_id++;

    auto msg = message::TCPBladeMessage::CreateTCPBladeMessage(
                                        *builder,
                                        txn_id,
                                        0,
                                        message::TCPBladeMessage::Message_Write,
                                        msg_contents.Union());
    builder->Finish(msg);

    return enqueue_message(builder, txn_id);
}

/**
  * Asynchronously reads an object corresponding to ObjectID
  * from the remote server.
  * @param id the id of the object the user wishes to read to local memory.
  * @param data a pointer to the buffer where the serialized object should
  * be read to.
  * @param size the size of the serialized object being read from
  * remote storage.
  * @return True if the object was successfully read from the server, false
  * otherwise.
  */
cirrus::BladeClient::ClientFuture TCPClient::read_async(ObjectID oid,
    void* data, uint64_t /* size */) {
    std::shared_ptr<flatbuffers::FlatBufferBuilder> builder =
                            std::make_shared<flatbuffers::FlatBufferBuilder>(
                                initial_buffer_size);

    // Create and send read request
    auto msg_contents = message::TCPBladeMessage::CreateRead(*builder, oid);

    const int txn_id = curr_txn_id++;

    auto msg = message::TCPBladeMessage::CreateTCPBladeMessage(
                                        *builder,
                                        txn_id,
                                        0,
                                        message::TCPBladeMessage::Message_Read,
                                        msg_contents.Union());
    builder->Finish(msg);

    return enqueue_message(builder, txn_id, data);
}

/**
  * Writes an object to remote storage under id.
  * @param id the id of the object the user wishes to write to remote memory.
  * @param data a pointer to the buffer where the serialized object should
  * be read read from.
  * @param size the size of the serialized object being read from
  * local memory.
  * @return True if the object was successfully written to the server, false
  * otherwise.
  */
bool TCPClient::write_sync(ObjectID id, const WriteUnit& w) {
    LOG<INFO>("Call to write_sync");
    auto future = write_async(id, w);
    LOG<INFO>("returned from write async");
    return future.get();
}

/**
  * Reads an object corresponding to ObjectID from the remote server.
  * @param id the id of the object the user wishes to read to local memory.
  * @param data a pointer to the buffer where the serialized object should
  * be read to.
  * @param size the size of the serialized object being read from
  * remote storage.
  * @return True if the object was successfully read from the server, false
  * otherwise.
  */
bool TCPClient::read_sync(ObjectID oid, void* data, uint64_t size) {
    LOG<INFO>("Call to read_sync.");
    auto future = read_async(oid, data, size);
    LOG<INFO>("Returned from read_async.");
    return future.get();
}

/**
  * Removes the object corresponding to the given ObjectID from the
  * remote store.
  * @param oid the ObjectID of the object to be removed.
  * @return True if the object was successfully removed from the server, false
  * if the object does not exist remotely or if another error occurred.
  */
bool TCPClient::remove(ObjectID oid) {
    std::shared_ptr<flatbuffers::FlatBufferBuilder> builder =
                            std::make_shared<flatbuffers::FlatBufferBuilder>(
                                initial_buffer_size);

    // Create and send removal request
    auto msg_contents = message::TCPBladeMessage::CreateRemove(*builder, oid);

    const int txn_id = curr_txn_id++;
    auto msg = message::TCPBladeMessage::CreateTCPBladeMessage(
                                    *builder,
                                    txn_id,
                                    0,
                                    message::TCPBladeMessage::Message_Remove,
                                    msg_contents.Union());
    builder->Finish(msg);

    auto future = enqueue_message(builder, txn_id);
    return future.get();
}


/**
  * Loop run by the thread that processes incoming messages. Contains all
  * logic for acting upon the incoming messages, which includes copying
  * serialized objects and notifying futures.
  */
void TCPClient::process_received() {
    // All elements stored on heap according to stack overflow, so it can grow
    std::vector<char> buffer;
    // Reserve the size of a 32 bit int
    int current_buf_size = sizeof(uint32_t);
    buffer.reserve(current_buf_size);
    int bytes_read = 0;

    /**
      * Message format
      * |---------------------------------------------------
      * | Msg size (4Bytes) | message content (flatbuffer) |
      * |---------------------------------------------------
      */

    while (1) {
        // Read in the size of the next message from the network
        LOG<INFO>("client waiting for message from server");
        bytes_read = 0;
        while (bytes_read < static_cast<int>(sizeof(uint32_t))) {
            int retval = read(sock, buffer.data() + bytes_read,
                              sizeof(uint32_t) - bytes_read);

            if (retval < 0) {
                if (errno == EINTR && terminate_threads == true) {
                    return;
                } else {
                    throw cirrus::Exception("Error when reading socket");
                }
            }

            bytes_read += retval;
            LOG<INFO>("Client read ", bytes_read, " bytes of 4");
        }
        // Convert to host byte order
        uint32_t *incoming_size_ptr = reinterpret_cast<uint32_t*>(
                                                                buffer.data());
        int incoming_size = ntohl(*incoming_size_ptr);

        LOG<INFO>("Size of incoming message received from server: ",
                  incoming_size);

        // Resize the buffer to be larger if necessary
        if (incoming_size > current_buf_size) {
            buffer.resize(incoming_size);
        }

        // Reset the counter to read in the flatbuffer
        bytes_read = 0;

        while (bytes_read < incoming_size) {
            int retval = read(sock, buffer.data() + bytes_read,
                              incoming_size - bytes_read);

            if (retval < 0) {
                throw cirrus::Exception("error while reading full message");
            }

            bytes_read += retval;
            LOG<INFO>("Client has read ", bytes_read, " of ", incoming_size,
                                                        " bytes.");
        }

        // Extract the flatbuffer from the receiving buffer
        auto ack = message::TCPBladeMessage::GetTCPBladeMessage(buffer.data());
        TxnID txn_id = ack->txnid();

        // obtain lock on map
        map_lock.wait();

        // find pair for this item in the map
        auto txn_pair = txn_map.find(txn_id);

        // ensure that the id really exists, error otherwise
        if (txn_pair == txn_map.end()) {
            LOG<ERROR>("The client received an unknown txn_id: ", txn_id);
            throw cirrus::Exception("Client error when processing "
                                     "Messages. txn_id received was invalid.");
        }

        // get the struct
        std::shared_ptr<struct txn_info> txn = txn_pair->second;

        // remove from map
        txn_map.erase(txn_pair);

        // release lock
        map_lock.signal();
        // Save the error code so that the future can read it
        *(txn->error_code) = static_cast<cirrus::ErrorCodes>(ack->error_code());
        LOG<INFO>("Error code read is: ", *(txn->error_code));
        // Process the ack
        switch (ack->message_type()) {
            case message::TCPBladeMessage::Message_WriteAck:
                {
                    // just put state in the struct, check for errors
                    *(txn->result) = ack->message_as_WriteAck()->success();
                    break;
                }
            case message::TCPBladeMessage::Message_ReadAck:
                {
                    /* Service the read request by sending the serialized object
                     to the client */
                    LOG<INFO>("Client processing ReadAck");
                    // copy the data from the ReadAck into the given pointer
                    *(txn->result) = ack->message_as_ReadAck()->success();
                    LOG<INFO>("Client wrote success");
                    auto data_fb_vector = ack->message_as_ReadAck()->data();
                    LOG<INFO>("Client has pointer to vector");
                    std::copy(data_fb_vector->begin(), data_fb_vector->end(),
                                reinterpret_cast<char*>(txn->mem_for_read));
                    LOG<INFO>("Client copied vector");
                    break;
                }
            case message::TCPBladeMessage::Message_RemoveAck:
                {
                    // put the result in the struct
                    *(txn->result) = ack->message_as_RemoveAck()->success();
                    break;
                }
            default:
                LOG<ERROR>("Unknown message", " type:", ack->message_type());
                exit(-1);
                break;
        }
        // Update the semaphore/CV so other know it is ready
        *(txn->result_available) = true;
        txn->sem->signal();
        LOG<INFO>("client done processing message");
    }
}

/**
 * Guarantees that an entire message is sent.
 * @param sock the fd of the socket to send on.
 * @param data a pointer to the data to send.
 * @param len the number of bytes to send.
 * @param flags for the send call.
 * @return the number of bytes sent.
 */
ssize_t TCPClient::send_all(int sock, const void* data, size_t len,
    int /* flags */) {
    uint64_t to_send = len;
    uint64_t total_sent = 0;
    int64_t sent = 0;

    while (to_send != total_sent) {
        sent = send(sock, data, len, 0);

        if (sent == -1) {
            throw cirrus::Exception("Server error sending data to client");
        }

        total_sent += sent;

        // Increment the pointer to data we're sending by the amount just sent
        data = static_cast<const char*>(data) + sent;
    }

    return total_sent;
}

/**
  * Loop run by the thread that handles sending messages. Takes
  * FlatBufferBuilders off of the queue and then sends the messages they
  * contain. Does not wait for response.
  */
void TCPClient::process_send() {
    // Wait until there are messages to send
    while (1) {
        queue_semaphore.wait();
        queue_lock.wait();

        if (terminate_threads) {
            return;
        }
        // This thread now owns the lock on the send queue

        // If a spurious wakeup, just continue
        if (send_queue.empty()) {
            queue_lock.signal();
            LOG<INFO>("Spurious wakeup.");
            continue;
        }
        // Take one item out of the send queue
        std::shared_ptr<flatbuffers::FlatBufferBuilder> builder =
            send_queue.front();
        send_queue.pop();
        int message_size = builder->GetSize();

        LOG<INFO>("Client sending size: ", message_size);
        // Convert size to network order and send
        uint32_t network_order_size = htonl(message_size);
        if (send(sock, &network_order_size, sizeof(uint32_t), 0)
                != sizeof(uint32_t)) {
            throw cirrus::Exception("Client error sending data to server");
        }

        LOG<INFO>("Client sending main message");
        // Send main message
        if (send_all(sock, builder->GetBufferPointer(), message_size, 0)
                != message_size) {
            throw cirrus::Exception("Client error sending data to server");
        }
        LOG<INFO>("message pair sent by client");

        // Release the lock so that the other thread may add to the send queue
        queue_lock.signal();

        // Add the builder to the queue if it is of the right type (a write)
        // And if not over capacity
        reuse_lock.wait();

        if (reuse_queue.size() < reuse_max) {
            // Clean the builder and reuse if it is the right type
            auto message_type = message::TCPBladeMessage::GetTCPBladeMessage(
                builder->GetBufferPointer())->message_type();

            if (message_type == message::TCPBladeMessage::Message_Write) {
                builder->Clear();
                reuse_queue.push(builder);
            }
        }
        reuse_lock.signal();
    }
}

/**
  * Given a message and optionally a pointer to memory, adds a message to the
  * send queue, adds a transaction to the map, and returns a future.
  * @param builder a shared_ptr to a FlatBufferBuilder, containing the
  * message.
  * @param An optional argument. Pointer to memory for read operations.
  * @return Returns a Future.
  */
cirrus::BladeClient::ClientFuture TCPClient::enqueue_message(
    std::shared_ptr<flatbuffers::FlatBufferBuilder> builder, int txn_id,
    void *ptr) {
    std::shared_ptr<struct txn_info> txn = std::make_shared<struct txn_info>();

    txn->mem_for_read = ptr;

    // Obtain lock on map
    map_lock.wait();

    // Add to map
    txn_map[txn_id] = txn;

    // Release lock on map
    map_lock.signal();

    // Build the future
    cirrus::BladeClient::ClientFuture future(txn->result,
        txn->result_available,
        txn->sem, txn->error_code);

    // Obtain lock on send queue
    queue_lock.wait();

    // Add builder to send queue
    send_queue.push(builder);

    // Release lock on send queue
    queue_lock.signal();
    // Alert that the queue has been updated
    queue_semaphore.signal();
    return future;
}

}  // namespace cirrus

#endif  // SRC_CLIENT_TCPCLIENT_H_
