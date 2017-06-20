#include "server/TCPServer.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <vector>

#include "utils/logging.h"  // TODO: remove word src
#include "src/common/schemas/TCPBladeMessage_generated.h"

namespace cirrus {

using TxnID = uint64_t;

static const int initial_buffer_size = 50;

/**
  * Constructor for the server. Given a port and queue length, sets the values
  * of the variables.
  */
TCPServer::TCPServer(int port, int queue_len) {
    port_ = port;
    queue_len_ = queue_len;
    server_sock_ = 0;
}

TCPServer::~TCPServer() {
}

/**
  * Initializer for the server. Sets up the socket it uses to listen for
  * incoming connections.
  */
void TCPServer::init() {
    struct sockaddr_in serv_addr;

    server_sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_ < 0)
        throw std::runtime_error("Error creating socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_);

    LOG<INFO>("Created socket in TCPServer");
    int opt = 1;
    if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        throw std::runtime_error("Error forcing port binding");
    }
    int ret = bind(server_sock_, reinterpret_cast<sockaddr*>(&serv_addr),
            sizeof(serv_addr));
    if (ret < 0)
        throw std::runtime_error("Error binding in port "
               + to_string(port_));

    listen(server_sock_, queue_len_);
}

/**
  * Server processing loop. When called, server loops infinitely, accepting
  * new connections and acting on messages received.
  */
void TCPServer::loop() {
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    // TODO: add support for multiple clients at once
    // TODO: stop closing socket?
    while (1) {
        int newsock = accept(server_sock_,
                reinterpret_cast<struct sockaddr*>(&cli_addr), &clilen);
        if (newsock < 0)
            throw std::runtime_error("Error accepting socket");

        process(newsock);

        close(newsock);
    }
}

/**
  * Process the message incoming on a particular socket. Reads in the message
  * from the socket, extracts the flatbuffer, and then acts depending on
  * the type of the message.
  * @param sock the file descriptor for the socket with an incoming message.
  */
void TCPServer::process(int sock) {
    LOG<INFO>("Processing socket: ", sock);
    std::vector<char> buffer;

    // Read in the incoming message

    // Reserve the size of a 32 bit int
    buffer.reserve(sizeof(uint32_t));
    int current_buf_size = sizeof(uint32_t);

    // TODO: replace with logging and exit
    int retval = read(sock, buffer.data(), sizeof(uint32_t));
    if (retval < 0) {
        printf("bad things happened when reading size from socket\n");
    }
    uint32_t *incoming_size_ptr = reinterpret_cast<uint32_t*>(buffer.data());
    int incoming_size = ntohl(*incoming_size_ptr);

    // Resize the buffer to be larger if necessary
    if (incoming_size > current_buf_size) {
        buffer.resize(incoming_size);
    }

    retval = read(sock, buffer.data(), incoming_size);

    // TODO: is this the correct way to check this?
    // TODO: replace with logging and exit
    if (retval < incoming_size) {
        printf("issue in reading socket. Full message not read. \n");
    }

    // Extract the message from the buffer
    auto msg = message::TCPBladeMessage::GetTCPBladeMessage(buffer.data());
    TxnID txn_id = msg->txnid();
    // Instantiate the builder
    flatbuffers::FlatBufferBuilder builder(initial_buffer_size);


    // Check message type
    switch (msg->message_type()) {
        case message::TCPBladeMessage::Message_Write:
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
                                    txn_id,
                                    message::TCPBladeMessage::Message_WriteAck,
                                    ack.Union());
    		builder.Finish(ack_msg);
                break;
            }
        case message::TCPBladeMessage::Message_Read:
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
                                    txn_id,
                                    message::TCPBladeMessage::Message_ReadAck,
                                    ack.Union());
    		builder.Finish(ack_msg);
                break;
            }
        case message::TCPBladeMessage::Message_Remove:
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
                                    txn_id,
                                    message::TCPBladeMessage::Message_RemoveAck,
                                    ack.Union());
    		builder.Finish(ack_msg);
                break;
            }
        default:
            LOG<ERROR>("Unknown message", " type:", msg->message_type());
            exit(-1);
            break;
    }

    int message_size = builder.GetSize();
    // Convert size to network order and send
    uint32_t network_order_size = htonl(message_size);
    send(sock, &network_order_size, sizeof(uint32_t), 0);

    // Send main message
    send(sock, builder.GetBufferPointer(), message_size, 0);
}

}  // namespace cirrus
