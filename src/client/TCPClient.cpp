#include "client/TCPClient.h"

#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string>
#include <vector>
#include <thread>
#include <utility>
#include <algorithm>
#include <memory>
#include <atomic>
#include "common/schemas/TCPBladeMessage_generated.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include "common/Exception.h"
#include "common/Synchronization.h"

namespace cirrus {

static const int initial_buffer_size = 50;

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
    // Ignore any sigpipes received, they will show as an error during
    // the read/write regardless, and ignoring will allow them to be better
    // handled/ for more information about the error to be known.
    signal(SIGPIPE, SIG_IGN);
    if (has_connected.exchange(true)) {
        LOG<INFO>("Client has previously connnected");
        return;
    }
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw cirrus::ConnectionException("Error when creating socket.");
    }
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

    // Connect to the server
    if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        throw cirrus::ConnectionException("Client could "
                                          "not connect to server.");
    }

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
  * @return A ClientFuture that contains information about the status of the
  * operation.
  */
BladeClient::ClientFuture TCPClient::write_async(ObjectID oid, const void* data,
                                    uint64_t size) {
    // Make sure that the pointer is not null
    TEST_Z(data);

#ifdef PERF_LOG
    TimerFunction builder_timer;
#endif
    // Create flatbuffer builder
    std::unique_ptr<flatbuffers::FlatBufferBuilder> builder =
                            std::make_unique<flatbuffers::FlatBufferBuilder>(
                                initial_buffer_size);

    // Create and send write request
    const int8_t *data_cast = reinterpret_cast<const int8_t*>(data);
    std::vector<int8_t> data_vector(data_cast, data_cast + size);
    auto data_fb_vector = builder->CreateVector(data_vector);
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


#ifdef PERF_LOG
    LOG<PERF>("TCPClient::write_async time to build message (us): ",
            builder_timer.getUsElapsed());
#endif

    return enqueue_message(std::move(builder), txn_id);
}

/**
 * Asynchronously reads an object corresponding to ObjectID
 * from the remote server.
 * @param id the id of the object the user wishes to read to local memory.
 * @return A ClientFuture containing information about the operation.
 */
BladeClient::ClientFuture TCPClient::read_async(ObjectID oid) {
#ifdef PERF_LOG
    TimerFunction builder_timer;
#endif
    std::unique_ptr<flatbuffers::FlatBufferBuilder> builder =
                            std::make_unique<flatbuffers::FlatBufferBuilder>(
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

#ifdef PERF_LOG
    LOG<PERF>("TCPClient::read_async time to build message (us): ",
            builder_timer.getUsElapsed());
#endif
    return enqueue_message(std::move(builder), txn_id);
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
bool TCPClient::write_sync(ObjectID oid, const void* data, uint64_t size) {
    LOG<INFO>("Call to write_sync");
    BladeClient::ClientFuture future = write_async(oid, data, size);
    LOG<INFO>("returned from write async");
    return future.get();
}

/**
  * Reads an object corresponding to ObjectID from the remote server.
  * @param id the id of the object the user wishes to read to local memory.
  * @return An std pair containing a shared pointer to the buffer that the
  * serialized object read from the server resides in as well as the size of
  * the buffer.
  */
std::pair<std::shared_ptr<char>, unsigned int>
TCPClient::read_sync(ObjectID oid) {
    LOG<INFO>("Call to read_sync.");
    BladeClient::ClientFuture future = read_async(oid);
    LOG<INFO>("Returned from read_async.");
    return future.getDataPair();
}

/**
  * Removes the object corresponding to the given ObjectID from the
  * remote store.
  * @param oid the ObjectID of the object to be removed.
  * @return True if the object was successfully removed from the server, false
  * if the object does not exist remotely or if another error occurred.
  */
bool TCPClient::remove(ObjectID oid) {
    std::unique_ptr<flatbuffers::FlatBufferBuilder> builder =
                            std::make_unique<flatbuffers::FlatBufferBuilder>(
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

    BladeClient::ClientFuture future = enqueue_message(std::move(builder),
        txn_id);
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
    int bytes_read = 0;  // XXX shouldn't this be an unsigned int?

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
                char *info = strerror(errno);
                LOG<ERROR>(info);
                if (errno == EINTR && terminate_threads == true) {
                    return;
                } else {
                    throw cirrus::Exception("Issue in reading socket. "
                                            "Full size not read");
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

#ifdef PERF_LOG
        TimerFunction receive_msg_time;
#endif
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
#ifdef PERF_LOG
        double receive_mbps = bytes_read / (1024 * 1024.0) /
            (receive_msg_time.getUsElapsed() / 1000.0 / 1000.0);
        LOG<PERF>("TCPClient::process_received rcv msg time (us): ",
                receive_msg_time.getUsElapsed(),
                " bw (MB/s): ", receive_mbps);
#endif

        // Extract the flatbuffer from the receiving buffer
        auto ack = message::TCPBladeMessage::GetTCPBladeMessage(buffer.data());
        TxnID txn_id = ack->txnid();

#ifdef PERF_LOG
        TimerFunction map_time;
#endif
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
#ifdef PERF_LOG
        LOG<PERF>("TCPClient::process_received map time (us): ",
                map_time.getUsElapsed());
#endif

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
                    *(txn->mem_size) = data_fb_vector->size();
                    *(txn->mem_for_read_ptr) = std::shared_ptr<char>(
                        new char[data_fb_vector->size()],
                        std::default_delete< char[]>());
                    LOG<INFO>("Client has pointer to vector");
                    // XXX we should get rid of this
                    std::copy(data_fb_vector->begin(), data_fb_vector->end(),
                                reinterpret_cast<char*>(
                                    (txn->mem_for_read_ptr->get())));
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
        std::unique_ptr<flatbuffers::FlatBufferBuilder> builder =
            std::move(send_queue.front());
        send_queue.pop();
        int message_size = builder->GetSize();

        LOG<INFO>("Client sending size: ", message_size);
        // Convert size to network order and send

        // XXX shouldn't this be a send_all?
        uint32_t network_order_size = htonl(message_size);
        if (send(sock, &network_order_size, sizeof(uint32_t), 0)
                != sizeof(uint32_t)) {
            throw cirrus::Exception("Client error sending data to server");
        }

#ifdef PERF_LOG
        TimerFunction send_time;
#endif
        LOG<INFO>("Client sending main message");
        // Send main message
        if (send_all(sock, builder->GetBufferPointer(), message_size, 0)
                != message_size) {
            throw cirrus::Exception("Client error sending data to server");
        }
#ifdef PERF_LOG
        double send_mbps = message_size / (1024 * 1024.0) /
            (send_time.getUsElapsed() / 1000.0 / 1000.0);
        LOG<PERF>("TCPClient::process_send send time (us): ",
                send_time.getUsElapsed(),
                " bw (MB/s): ", send_mbps);
#endif
        LOG<INFO>("message pair sent by client");

        // Release the lock so that the other thread may add to the send queue
        queue_lock.signal();
    }
}

/**
  * Given a message and optionally a pointer to memory, adds a message to the
  * send queue, adds a transaction to the map, and returns a future.
  * @param builder a unique_ptr to a FlatBufferBuilder, containing the
  * message.
  * @param txn_id transaction id corresponding to the event being enqueued.
  * @return Returns a Future.
  */
BladeClient::ClientFuture TCPClient::enqueue_message(
            std::unique_ptr<flatbuffers::FlatBufferBuilder> builder,
            const int txn_id) {
#ifdef PERF_LOG
    TimerFunction enqueue_time;
#endif
    std::shared_ptr<struct txn_info> txn = std::make_shared<struct txn_info>();

    // Obtain lock on map
    map_lock.wait();

    // Add to map
    txn_map[txn_id] = txn;

    // Release lock on map
    map_lock.signal();

    // Build the future
    BladeClient::ClientFuture future(txn->result, txn->result_available,
                          txn->sem, txn->error_code, txn->mem_for_read_ptr,
                          txn->mem_size);

    // Obtain lock on send queue
    queue_lock.wait();

    // Add builder to send queue
    send_queue.push(std::move(builder));

    // Release lock on send queue
    queue_lock.signal();
    // Alert that the queue has been updated
    queue_semaphore.signal();

#ifdef PERF_LOG
    LOG<PERF>("TCPClient::enqueue_message enqueue time (us): ",
            enqueue_time.getUsElapsed());
#endif
    return future;
}

}  // namespace cirrus
