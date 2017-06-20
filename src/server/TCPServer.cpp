#include "src/server/TCPServer.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <vector>

#include "src/utils/logging.h"  // TODO: remove word src
#include "src/common/schemas/TCPBladeMessage_generated.h"

namespace cirrus {

static const int initial_buffer_size = 50;

TCPServer::TCPServer(int port, int queue_len) {
    port_ = port;
    queue_len_ = queue_len;
    server_sock_ = 0;
}

TCPServer::~TCPServer() {
}

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

void TCPServer::process(int sock) {
    LOG<INFO>("Processing socket: ", sock);
    char buffer[1024] = {0};
    int retval = read(sock, buffer, 1024);
    if (retval < 0) {
        printf("bad things happened when reading from socket\n");
    }

    auto msg = message::TCPBladeMessage::GetTCPBladeMessage(buffer);


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
                                      message::TCPBladeMessage::Message_WriteAck,
                                      ack.Union());
                builder.Finish(ack_msg);
                int message_size = builder.GetSize();
                send(sock, builder.GetBufferPointer(), message_size, 0);
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
                                     message::TCPBladeMessage::Message_ReadAck,
                                     ack.Union());

                builder.Finish(ack_msg);
                int message_size = builder.GetSize();
                send(sock, builder.GetBufferPointer(), message_size, 0);
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
}

}  // namespace cirrus
