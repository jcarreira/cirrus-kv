#include "client/TCPClient.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>
#include <memory>
#include "common/schemas/TCPBladeMessage_generated.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include "common/Future.h"

namespace cirrus {

static const int initial_buffer_size = 50;

/**
  * Connects the client to the remote server. Opens a socket and attempts to
  * connect on the given address and port.
  * @param address the ipv4 address of the server, represented as a string
  * @param port_string the port to connect to, represented as a string
  */
void TCPClient::connect(std::string address, std::string port_string) {
    // Create socket

    // TODO: Replace with logged errors and exit
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("socket creation error \n \n");
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
    // TODO: Replace with logged errors and exit
    if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      printf("could not connect to server\n\n");
    }

    receiver_thread = std::thread(&TCPClient::process_received, this);
    sender_thread = std::thread(&TCPClient::process_send, this);
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
cirrus::Future TCPClient::write_async(ObjectID oid, void* data, uint64_t size) {
    // Make sure that the pointer is not null
    TEST_NZ(data == nullptr);
    // Create flatbuffer builder
    std::shared_ptr<flatbuffers::FlatBufferBuilder> builder =
                            std::make_shared<flatbuffers::FlatBufferBuilder>(
                                initial_buffer_size);

    // Create and send write request
    int8_t *data_cast = reinterpret_cast<int8_t*>(data);
    std::vector<int8_t> data_vector(data_cast, data_cast + size);
    auto data_fb_vector = builder->CreateVector(data_vector);
    auto msg_contents = message::TCPBladeMessage::CreateWrite(*builder,
                                                              oid,
                                                              data_fb_vector);
    auto msg = message::TCPBladeMessage::CreateTCPBladeMessage(
                                        *builder,
                                        curr_txn_id,
                                        message::TCPBladeMessage::Message_Write,
                                        msg_contents.Union());
    builder->Finish(msg);

    return enqueue_message(builder);
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
cirrus::Future TCPClient::read_async(ObjectID oid, void* data,
                                     uint64_t /* size */) {
    std::shared_ptr<flatbuffers::FlatBufferBuilder> builder =
                            std::make_shared<flatbuffers::FlatBufferBuilder>(
                                initial_buffer_size);

    // Create and send read request

    auto msg_contents = message::TCPBladeMessage::CreateRead(*builder, oid);

    auto msg = message::TCPBladeMessage::CreateTCPBladeMessage(
                                        *builder,
                                        curr_txn_id,
                                        message::TCPBladeMessage::Message_Read,
                                        msg_contents.Union());
    builder->Finish(msg);

    return enqueue_message(builder, data);
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
bool TCPClient::write_sync(ObjectID oid, void* data, uint64_t size) {
    cirrus::Future future = write_async(oid, data, size);
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
    cirrus::Future future = read_async(oid, data, size);
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

    auto msg = message::TCPBladeMessage::CreateTCPBladeMessage(
                                    *builder,
                                    curr_txn_id,
                                    message::TCPBladeMessage::Message_Remove,
                                    msg_contents.Union());
    builder->Finish(msg);

    cirrus::Future future = enqueue_message(builder);
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
    buffer.reserve(sizeof(uint32_t));
    int current_buf_size = sizeof(uint32_t);

    while (1) {
        // Read in the size of the next message from the network
        int retval = read(sock, buffer.data(), sizeof(uint32_t));

        if (retval < static_cast<int>(sizeof(uint32_t))) {
            printf("issue in reading socket. Full size not read. \n");
        }
        // Convert to host byte order
        uint32_t *incoming_size_ptr = reinterpret_cast<uint32_t*>(
                                                                buffer.data());
        int incoming_size = ntohl(*incoming_size_ptr);

        // Resize the buffer to be larger if necessary
        if (incoming_size > current_buf_size) {
            buffer.resize(incoming_size);
        }

        // Read rest of message into buffer
        retval = read(sock, buffer.data(), incoming_size);

        // TODO: is this the correct way to check this?
        if (retval < incoming_size) {
            printf("issue in reading socket. Full message not read. \n");
        }


        auto ack = message::TCPBladeMessage::GetTCPBladeMessage(buffer.data());
        TxnID txn_id = ack->txnid();

        // obtain lock on map
        std::unique_lock<std::mutex> map_lock(txn_map_mutex);
        map_lock.lock();

        // find pair for this item in the map
        auto txn_pair = txn_map.find(txn_id);

        // ensure that the id really exists, error otherwise
        if (txn_pair == txn_map.end()) {
            // error
        }

        // get the struct
        std::shared_ptr<struct txn_info> txn = txn_pair->second;

        // remove from map
        txn_map.erase(txn_id);

        // release lock
        map_lock.unlock();

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

                    // copy the data from the ReadAck into the given pointer
                    *(txn->result) = ack->message_as_ReadAck()->success();
                    auto data_fb_vector = ack->message_as_ReadAck()->data();
                    std::copy(data_fb_vector->begin(), data_fb_vector->end(),
                                reinterpret_cast<char*>(txn->mem_for_read));

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
        txn->sem->signal();
    }
}

/**
  * Loop run by the thread that handles sending messages. Takes
  * FlatBufferBuilders off of the queue and then sends the messages they
  * contain. Does not wait for response.
  */
void TCPClient::process_send() {
    // TODO: switch to just locks
    // Wait until there are messages to send
    while (1) {
        std::unique_lock<std::mutex> lk(send_queue_mutex);
        send_queue_cv.wait(lk);

        // This thread now owns the lock on the send queue

        // Process the send queue until it is empty
        while (send_queue.size() != 0) {
            std::shared_ptr<flatbuffers::FlatBufferBuilder> builder =
                                                            send_queue.front();
            send_queue.pop();
            int message_size = builder->GetSize();

            // Convert size to network order and send
            uint32_t network_order_size = htonl(message_size);
            send(sock, &network_order_size, sizeof(uint32_t), 0);

            // Send main message
            send(sock, builder->GetBufferPointer(), message_size, 0);
        }
        // Release the lock so that the other thread may add to the send queue
        lk.unlock();
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
cirrus::Future TCPClient::enqueue_message(
            std::shared_ptr<flatbuffers::FlatBufferBuilder> builder,
            void *ptr) {
    std::shared_ptr<struct txn_info> txn = std::make_shared<struct txn_info>();

    txn->mem_for_read = ptr;

    // TODO: change this to use the cirrus lock?
    // spin lock may have lower latency

    // Obtain lock on map
    std::unique_lock<std::mutex> map_lock(txn_map_mutex);
    map_lock.lock();

    // Add to map
    txn_map[curr_txn_id++] = txn;

    // Release lock on map
    map_lock.release();

    // Build the future
    cirrus::Future future(txn->result, txn->sem);

    // Obtain lock on send queue
    std::unique_lock<std::mutex> queue_lock(send_queue_mutex);
    queue_lock.lock();

    // Add builder to send queue
    send_queue.push(builder);

    // Release lock on send queue
    queue_lock.release();
    return future;
}

}  // namespace cirrus