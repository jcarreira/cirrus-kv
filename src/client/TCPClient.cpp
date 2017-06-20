#include "src/client/TCPClient.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <thread>
#include "src/common/schemas/TCPBladeMessage_generated.h"

namespace cirrus {

static const int initial_buffer_size = 50;

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
    std::shared_ptr<flatbuffers::FlatBufferBuilder> builder =
                            std::make_shared<flatbuffers::FlatBufferBuilder>(
                                initial_buffer_size);

    // Create and send write request
    int8_t *data_cast = reinterpret_cast<int8_t*>(data);
    std::vector<int8_t> data_vector(data_cast, data_cast + size);
    auto data_fb_vector = builder->CreateVector(data_vector);
    auto msg_contents = message::TCPBladeMessage::CreateWrite(*builder, oid, data_fb_vector);
    auto msg = message::TCPBladeMessage::CreateTCPBladeMessage(
                                        *builder,
                                        message::TCPBladeMessage::Message_Write,
                                        msg_contents.Union());
    builder->Finish(msg);

    std::shared_ptr<struct txn_info> txn = std::make_shared<struct txn_info>();


    // TODO: change this to use the cirrus lock?
    // spin lock may have lower latency


    // Obtain lock on map
    std::unique_lock<std::mutex> map_lock(txn_map_mutex);
    map_lock.lock();
    // Add to map
    txn_map[curr_txn_id++] = txn;

    // Release lock on map
    map_lock.release();

    // Obtain lock on send queue
    std::unique_lock<std::mutex> queue_lock(send_queue_mutex);
    queue_lock.lock();

    // Add to send queue
    send_queue.push(builder);

    // Release lock
    queue_lock.release();
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
bool TCPClient::read_sync(ObjectID /*id*/, void* /*data*/, uint64_t /*size*/) {
    return true;
}

/**
  * Removes the object corresponding to the given ObjectID from the remote store.
  * @return True if the object was successfully removed from the server, false
  * if the object does not exist remotely or if another error occurred.
  */
bool TCPClient::remove(ObjectID /*id*/) {
    return true;
}

void TCPClient::process_received() {
    // All elements stored on heap according to stack overflow, so it can grow
    std::vector<char> buffer;
    // Reserve the size of a 32 bit int
    buffer.reserve(sizeof(uint32_t));
    int current_buf_size = sizeof(uint32_t);

    while (1) {
        // Read in the size of the next message from the network
        int retval = read(sock, buffer.data(), sizeof(uint32_t));

        if (retval < sizeof(uint32_t)) {
            printf("issue in reading socket. Full size not read. \n");
        }
        // Convert to host byte order
        int incoming_size = htonl(reinterpret_cast<uint32_t>(buffer.data()));

        // Resize the buffer to be larger if necessary
        if (incoming_size > current_buf_size) {
            buffer.resize(size);
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

        // find pair for this item in the map
        auto txn_pair =

        // ensure that the id really exists, error otherwise


        // Process the ack
        switch (ack->message_type()) {
            case message::TCPBladeMessage::Message_WriteAck:
                {
                    /* Service the write request by storing the serialized object */
                    ObjectID oid = msg->message_as_Write()->oid();
                    auto data_fb = msg->message_as_Write()->data();
    		std::vector<uint8_t> data(data_fb->begin(), data_fb->end());
                    // Create entry in store mapping the data to the id
                    store[oid] = data;

                    // Create and send ack
                    auto ack = message::TCPBladeMessage::CreateWriteAck(builder,
                                               true, oid);
                    auto ack_msg =
                         message::TCPBladeMessage::CreateTCPBladeMessage(builder,
                                          message::TCPBladeMessage::Message_WriteAck,
                                          ack.Union());
                    builder.Finish(ack_msg);
                    int message_size = builder.GetSize();
                    send(sock, builder.GetBufferPointer(), message_size, 0);
                    break;
                }
            case message::TCPBladeMessage::Message_ReadAck:
                {
                    /* Service the read request by sending the serialized object
                     to the client */

                    ObjectID oid = msg->message_as_Write()->oid();

                    bool exists = true;
                    auto entry_itr = store.find(oid);
                    if (entry_itr != store.end()) {
                        exists = false;
                    }

                    if (exists) {
                        auto data = store[oid];
                    } else {
                        std::vector<int8_t> data;
                    }

                    // Create and send ack
                    auto ack = message::TCPBladeMessage::CreateReadAck(builder,
                                                oid, exists);
                    auto ack_msg =
                        message::TCPBladeMessage::CreateTCPBladeMessage(builder,
                                         message::TCPBladeMessage::Message_ReadAck,
                                         ack.Union());

                    builder.Finish(ack_msg);
                    int message_size = builder.GetSize();
                    send(sock, builder.GetBufferPointer(), message_size, 0);
                    break;
                }
            case message::TCPBladeMessage::Message_RemoveAck:
                {
                  ObjectID oid = msg->message_as_Write()->oid();

                    bool success = false;
                    auto entry_itr = store.find(oid);
                    if (entry_itr != store.end()) {
                        store.erase(entry_itr);
                        success = true;
                    }
                    // Create and send ack
                    auto ack = message::TCPBladeMessage::CreateWriteAck(builder,
                                             oid, success);
                    auto ack_msg =
                       message::TCPBladeMessage::CreateTCPBladeMessage(builder,
                                        message::TCPBladeMessage::Message_RemoveAck,
                                        ack.Union());
                    builder.Finish(ack_msg);
                    int message_size = builder.GetSize();
                    send(sock, builder.GetBufferPointer(), message_size, 0);
                    break;
                }
            default:
                LOG<ERROR>("Unknown message", " type:", msg->message_type());
                exit(-1);
                break;
        }


        // TODO: check the type of the message first
        // TODO: check for error messages?
        return ack->message_as_WriteAck()->success();
    }
}

void TCPClient::process_send() {
    // TODO: switch to just locks
    // Wait until there are messages to send
    while (1) {
        std::unique_lock<std::mutex> lk(send_queue_mutex);
        send_queue_cv.wait(lk);

        // This thread now owns the lock on the send queue

        // Process the send queue until it is empty
        while (send_queue.size() != 0) {
            auto builder = send_queue.pop();
            int message_size = builder->GetSize();
            send(sock, builder->GetBufferPointer(), message_size, 0);
        }
        // Release the lock so that the other thread may add to the send queue
        lk.unlock();
    }
}


void TCPClient::test() {
    std::string hello = "Hello from client";
    send(sock, hello.c_str(), hello.length(), 0);
    char buffer[1024] = {0};
    int retval = read(sock, buffer, 1024);
    if (retval < 0) {
        printf("issue in receiving\n");
    }
    printf("%s\n", buffer);
}
}  // namespace cirrus
