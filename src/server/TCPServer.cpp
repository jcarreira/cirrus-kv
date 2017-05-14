/* Copyright 2016 Joao Carreira */

#include "src/server/TCPServer.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include "src/utils/logging.h"

namespace cirrus {

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

        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_family =  INADDR_ANY;
        serv_addr.sin_port = htons(port_);

        LOG<INFO>("Created socket in TCPServer");

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
    sock = 3;  // warning
    LOG<INFO>("Processing socket: ", sock);
}

}  // namespace cirrus
