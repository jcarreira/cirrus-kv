#include "server/TCPServer.h"

#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include "utils/logging.h"
#include "common/Exception.h"
#include "common/schemas/TCPBladeMessage_generated.h"

namespace cirrus {

using TxnID = uint64_t;

// size for Flatbuffer's buffer
static const int initial_buffer_size = 50;

/**
  * Constructor for the server. Given a port and queue length, sets the values
  * of the variables.
  * @param port the port the server will listen on
  * @param queue_len the length of the queue to make connections with the
  * server.
  * @param max_fds_ the maximum number of clients that can be connected to the
  * server at the same time.
  * @param pool_size_ the number of bytes to have in the memory pool.
  */
TCPServer::TCPServer(int port, uint64_t pool_size_,
    uint64_t num_threads, uint64_t max_fds_) :
    port_(port), num_threads(num_threads),
    pool_size(pool_size_), max_fds(max_fds_ + 1) {
        if (max_fds_ + 1 == 0) {
            throw cirrus::Exception("Max_fds value too high, "
                "overflow occurred.");
        }

        for (unsigned int i = 0; i < num_threads; i++) {
            threads_vector.push_back(
                std::move(std::make_unique<std::thread>(
                    &TCPServer::wait_to_process,
                    this)));
        }
    }

void TCPServer::wait_to_process() {
    while (1) {
        queue_semaphore.wait();
        queue_lock.wait();
        if (!process_queue.empty()) {
            waiting_threads--;
            std::reference_wrapper<struct pollfd> to_process_ref =
                process_queue.front();
            process_queue.pop();
            // Set the fd to be ignored on future calls to poll
            struct pollfd& to_process_struct = to_process_ref.get();
            int to_process_fd = to_process_struct.fd;
            to_process_struct.fd *= -1;

            queue_lock.signal();
            if (process(to_process_fd)) {
                to_process_struct.fd *= -1;
            } else {
                LOG<INFO>("Processing failed on socket: ",
                    to_process_struct.fd);
                // do not make future alerts on this fd
                to_process_struct.fd = -1;
            }
            to_process_struct.revents = 0;
            waiting_threads++;
        } else {
            LOG<INFO>("Spurious wakeup");
            queue_lock.signal();
        }
    }
}

/**
  * Initializer for the server. Sets up the socket it uses to listen for
  * incoming connections.
  */
void TCPServer::init() {
    struct sockaddr_in serv_addr;

    server_sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_ < 0) {
        throw cirrus::ConnectionException("Server error creating socket");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_);

    LOG<INFO>("Created socket in TCPServer");

    int opt = 1;
    if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR, &opt,
                   sizeof(opt))) {
        switch (errno) {
            case EBADF:
                LOG<ERROR>("EBADF");
                break;
            case ENOTSOCK:
                LOG<ERROR>("ENOTSOCK");
                break;
            case ENOPROTOOPT:
                LOG<ERROR>("ENOPROTOOPT");
                break;
            case EFAULT:
                LOG<ERROR>("EFAULT");
                break;
            case EDOM:
                LOG<ERROR>("EDOM");
                break;
        }
        throw cirrus::ConnectionException("Error forcing port binding");
    }

    if (setsockopt(server_sock_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt))) {
        throw cirrus::ConnectionException("Error setting socket options.");
    }

    if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        switch (errno) {
            case EBADF:
                LOG<ERROR>("EBADF");
                break;
            case ENOTSOCK:
                LOG<ERROR>("ENOTSOCK");
                break;
            case ENOPROTOOPT:
                LOG<ERROR>("ENOPROTOOPT");
                break;
            case EFAULT:
                LOG<ERROR>("EFAULT");
                break;
            case EDOM:
                LOG<ERROR>("EDOM");
                break;
        }
        throw cirrus::ConnectionException("Error forcing port binding");
    }

    int ret = bind(server_sock_, reinterpret_cast<sockaddr*>(&serv_addr),
            sizeof(serv_addr));
    if (ret < 0) {
        throw cirrus::ConnectionException("Error binding in port "
               + to_string(port_));
    }

    // SOMAXCONN is the "max reasonable backlog size" defined in socket.h
    if (listen(server_sock_, SOMAXCONN) == -1) {
        throw cirrus::ConnectionException("Error listening on port "
            + to_string(port_));
    }

    fds.at(curr_index).fd = server_sock_;
    // Only listen for data to read
    fds.at(curr_index++).events = POLLIN;
}

/**
 * Function passed into std::remove_if
 * @param x a struct pollfd being examined
 * @return True if the struct should be removed. This is true if the fd is -1,
 * meaning that it is being ignored.
 */
bool TCPServer::testRemove(struct pollfd x) {
    // If this pollfd will be removed, the index of the next location to insert
    // should be reduced by one correspondingly.
    if (x.fd == -1) {
        curr_index -= 1;
    }
    return x.fd == -1;
}
/**
  * Server processing loop. When called, server loops infinitely, accepting
  * new connections and acting on messages received.
  */
void TCPServer::loop() {
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    while (1) {
        LOG<INFO>("Server calling poll.");
        int poll_status = poll(fds.data(), curr_index, timeout);
        LOG<INFO>("Poll returned with status: ", poll_status);

        if (poll_status == -1) {
            throw cirrus::ConnectionException("Server error calling poll.");
        } else if (poll_status == 0) {
            LOG<INFO>(timeout, " milliseconds elapsed without contact.");
        } else {
            // there is at least one pending event, find it.
            for (uint64_t i = 0; i < curr_index; i++) {
                struct pollfd& curr_fd = fds.at(i);
                // Ignore the fd if we've said we don't care about it
                if (curr_fd.fd == -1) {
                    continue;
                }
                if (curr_fd.revents != POLLIN) {
                    LOG<ERROR>("Non read event on socket: ", curr_fd.fd);
                    if (curr_fd.revents & POLLHUP) {
                        LOG<INFO>("Connection was closed by client");
                        LOG<INFO>("Closing socket: ", curr_fd.fd);
                        close(curr_fd.fd);
                        curr_fd.fd = -1;
                    }

                } else if (curr_fd.fd == server_sock_) {
                    LOG<INFO>("New connection incoming");

                    // New data on main socket, accept and connect
                    // TODO(Tyler): loop this to accept multiple at once?
                    // TODO(Tyler): Switch to non blocking sockets?
                    int newsock = accept(server_sock_,
                            reinterpret_cast<struct sockaddr*>(&cli_addr),
                            &clilen);
                    if (newsock < 0) {
                        throw std::runtime_error("Error accepting socket");
                    }
                    // If at capacity, reject connection
                    if (curr_index == max_fds) {
                        close(newsock);
                    } else {
                        LOG<INFO>("Created new socket: ", newsock);
                        fds.at(curr_index).fd = newsock;
                        fds.at(curr_index).events = POLLIN;
                        curr_index++;
                    }
                    curr_fd.revents = 0;  // Reset the event flags
                } else {
                    queue_lock.wait();
                    process_queue.push(
                        std::reference_wrapper<struct pollfd>(curr_fd));
                    queue_lock.signal();
                    queue_semaphore.signal();
                }
            }
        }
        // If at max capacity, try to make room
        // lock all threads out of the processing queue
        queue_lock.wait();

        // wait till no threads are processing
        while (waiting_threads != num_threads) {
        }
        if (curr_index == max_fds) {
            // Try to purge unused fds, those with fd == -1
            std::remove_if(fds.begin(), fds.end(),
                std::bind(&TCPServer::testRemove, this,
                    std::placeholders::_1));
        }
        // let the threads run again
        queue_lock.signal();
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
ssize_t TCPServer::send_all(int sock, const void* data, size_t len,
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
  * Read header from client's message or return false if client has disconnected
  * @param buffer Buffer where data is stored
  * @param sock Socket used for communication
  * @param bytes_read Keeps track of how many bytes have been read
  * @param Return false if client disconnected, true otherwise
  */
bool TCPServer::read_from_client(
        std::vector<char>& buffer, int sock, int& bytes_read) {
    bool first_loop = true;
    while (bytes_read < static_cast<int>(sizeof(uint32_t))) {
        int retval = read(sock, buffer.data() + bytes_read,
                          sizeof(uint32_t) - bytes_read);

        if (first_loop && retval == 0) {
            // Socket is closed by client if 0 bytes are available
            close(sock);
            LOG<INFO>("Closing socket: ", sock);
            return false;
        }

        if (retval < 0) {
            throw cirrus::Exception("Server issue in reading "
                                    "socket during size read.");
        }

        bytes_read += retval;
        first_loop = false;
    }
    return true;
}

/**
  * Process the message incoming on a particular socket. Reads in the message
  * from the socket, extracts the flatbuffer, and then acts depending on
  * the type of the message.
  * @param sock the file descriptor for the socket with an incoming message.
  */
bool TCPServer::process(int sock) {
    LOG<INFO>("Processing socket: ", sock);
    std::vector<char> buffer;

    // Read in the incoming message

    // Reserve the size of a 32 bit int
    int current_buf_size = sizeof(uint32_t);
    buffer.reserve(current_buf_size);
    int bytes_read = 0;

    bool ret = read_from_client(buffer, sock, bytes_read);
    if (!ret) {
        return false;
    }

    LOG<INFO>("Server received size from client");
    // Convert to host byte order
    uint32_t* incoming_size_ptr = reinterpret_cast<uint32_t*>(
                                                            buffer.data());
    int incoming_size = ntohl(*incoming_size_ptr);
    LOG<INFO>("Server received incoming size of ", incoming_size);
    // Resize the buffer to be larger if necessary
#ifdef PERF_LOG
    TimerFunction resize_time;
#endif
    if (incoming_size > current_buf_size) {
        buffer.resize(incoming_size);
    }
#ifdef PERF_LOG
    LOG<PERF>("TCPServer::process resize time (us): ",
            resize_time.getUsElapsed());
#endif

    bytes_read = 0;

#ifdef PERF_LOG
    TimerFunction receive_time;
#endif
    // XXX shouldn't this be in a recv_all?
    while (bytes_read < incoming_size) {
        int retval = read(sock, buffer.data() + bytes_read,
                          incoming_size - bytes_read);

        if (retval < 0) {
            throw cirrus::Exception("Serverside error while "
                                    "reading full message.");
        }

        bytes_read += retval;
        LOG<INFO>("Server received ", bytes_read, " bytes of ", incoming_size);
    }
#ifdef PERF_LOG
    double recv_mbps = bytes_read / (1024.0 * 1024) /
        (receive_time.getUsElapsed() / 1000000.0);
    LOG<PERF>("TCPServer::process receive time (us): ",
            receive_time.getUsElapsed(),
            " bw (MB/s): ", recv_mbps);
#endif
    LOG<INFO>("Server received full message from client");

    // Extract the message from the buffer
    auto msg = message::TCPBladeMessage::GetTCPBladeMessage(buffer.data());
    TxnID txn_id = msg->txnid();
    // Instantiate the builder
    flatbuffers::FlatBufferBuilder builder(initial_buffer_size);

    // Initialize the error code
    cirrus::ErrorCodes error_code = cirrus::ErrorCodes::kOk;

    store_lock.wait();
    LOG<INFO>("Server checking type of message");
    // Check message type
    bool success = true;
    switch (msg->message_type()) {
        case message::TCPBladeMessage::Message_Write:
            {
#ifdef PERF_LOG
                TimerFunction write_time;
#endif
                LOG<INFO>("Server processing write request.");

                // first see if the object exists on the server.
                // If so, overwrite it and account for the size change.
                ObjectID oid = msg->message_as_Write()->oid();

                auto entry_itr = store.find(oid);
                if (entry_itr != store.end()) {
                    curr_size -= entry_itr->second.size();
                }

                // Throw error if put would exceed size of the store
                auto data_fb = msg->message_as_Write()->data();
                if (curr_size + data_fb->size() > pool_size) {
                    LOG<ERROR>("Put would go over capacity on server. ",
                                "Current size: ", curr_size,
                                " Incoming size: ", data_fb->size(),
                                " Pool size: ", pool_size);
                    error_code =
                        cirrus::ErrorCodes::kServerMemoryErrorException;
                    success = false;
                } else {
                    // Service the write request by
                    //  storing the serialized object
                    std::vector<int8_t> data(data_fb->begin(), data_fb->end());
                    // Create entry in store mapping the data to the id
                    curr_size += data_fb->size();
                    store[oid] = data;
                }

                // Create and send ack
                auto ack = message::TCPBladeMessage::CreateWriteAck(builder,
                                           oid, success);
                auto ack_msg =
                     message::TCPBladeMessage::CreateTCPBladeMessage(builder,
                                    txn_id,
                                    static_cast<int64_t>(error_code),
                                    message::TCPBladeMessage::Message_WriteAck,
                                    ack.Union());
                builder.Finish(ack_msg);
#ifdef PERF_LOG
                double write_mbps = data_fb->size() / (1024.0 * 1024) /
                    (write_time.getUsElapsed() / 1000000.0);
                LOG<PERF>("TCPServer::process write time (us): ",
                        write_time.getUsElapsed(),
                        " bw (MB/s): ", write_mbps,
                        " size: ", data_fb->size());
#endif
                break;
            }
        case message::TCPBladeMessage::Message_Read:
            {
#ifdef PERF_LOG
                TimerFunction read_time;
#endif
                /* Service the read request by sending the serialized object
                 to the client */
                LOG<INFO>("Processing read request");
                ObjectID oid = msg->message_as_Read()->oid();
                LOG<INFO>("Server extracted oid");
                auto entry_itr = store.find(oid);
                LOG<INFO>("Got pair from store");
                // If the oid is not on the server, this operation has failed
                if (entry_itr == store.end()) {
                    success = false;
                    error_code = cirrus::ErrorCodes::kNoSuchIDException;
                    LOG<ERROR>("Oid ", oid, " does not exist on server");
                }
                flatbuffers::Offset<flatbuffers::Vector<int8_t>> fb_vector;
                if (success) {
                    fb_vector = builder.CreateVector(entry_itr->second);
                } else {
                    std::vector<int8_t> data;
                    fb_vector = builder.CreateVector(data);
                }

                LOG<INFO>("Server building response");
                // Create and send ack
                auto ack = message::TCPBladeMessage::CreateReadAck(builder,
                                            oid, success, fb_vector);
                auto ack_msg =
                    message::TCPBladeMessage::CreateTCPBladeMessage(builder,
                                    txn_id,
                                    static_cast<int64_t>(error_code),
                                    message::TCPBladeMessage::Message_ReadAck,
                                    ack.Union());
                builder.Finish(ack_msg);
                LOG<INFO>("Server done building response");
#ifdef PERF_LOG
                double read_mbps = entry_itr->second.size() / (1024.0 * 1024) /
                    (read_time.getUsElapsed() / 1000000.0);
                LOG<PERF>("TCPServer::process read time (us): ",
                        read_time.getUsElapsed(),
                        " bw (MB/s): ", read_mbps,
                        " size: ", entry_itr->second.size());
#endif
                break;
            }
        case message::TCPBladeMessage::Message_Remove:
            {
                ObjectID oid = msg->message_as_Remove()->oid();

                success = false;
                auto entry_itr = store.find(oid);
                // Remove the object if it exists on the server.
                if (entry_itr != store.end()) {
                    store.erase(entry_itr);
                    curr_size -= entry_itr->second.size();
                    success = true;
                }
                // Create and send ack
                auto ack = message::TCPBladeMessage::CreateWriteAck(builder,
                                         oid, success);
                auto ack_msg =
                   message::TCPBladeMessage::CreateTCPBladeMessage(builder,
                                    txn_id,
                                    static_cast<int64_t>(error_code),
                                    message::TCPBladeMessage::Message_RemoveAck,
                                    ack.Union());
                builder.Finish(ack_msg);
                break;
            }
        default:
            LOG<ERROR>("Unknown message", " type:", msg->message_type());
            throw cirrus::Exception("Unknown message "
                                    "type received from client.");
            break;
    }

    store_lock.signal();
    int message_size = builder.GetSize();
    // Convert size to network order and send
    uint32_t network_order_size = htonl(message_size);
    if (send(sock, &network_order_size, sizeof(uint32_t), 0) == -1) {
        LOG<ERROR>("Server error sending message back to client. "
            "Possible client died");
        return false;
    }

    LOG<INFO>("Server sent size.");
    LOG<INFO>("On server error code is: ", static_cast<int64_t>(error_code));
    // Send main message
#ifdef PERF_LOG
    TimerFunction reply_time;
#endif
    if (send_all(sock, builder.GetBufferPointer(), message_size, 0)
        != message_size) {
        LOG<ERROR>("Server error sending message back to client. "
            "Possible client died");
        return false;
    }
#ifdef PERF_LOG
    double reply_mbps = message_size / (1024.0 * 1024) /
        (reply_time.getUsElapsed() / 1000000.0);
    LOG<PERF>("TCPServer::process reply time (us): ",
            reply_time.getUsElapsed(),
            " bw (MB/s): ", reply_mbps);
#endif

    LOG<INFO>("Server sent ack of size: ", message_size);
    LOG<INFO>("Server done processing message from client");
    return true;
}

}  // namespace cirrus
