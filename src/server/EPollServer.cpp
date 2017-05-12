/* Copyright 2016 Joao Carreira */

#include "src/server/TCPServer.h"


namespace sirius {

TCPServer::TCPServer(int port, int queue_len = 100) {
    port_ = port;
   queue_len_ = queue_len; 
}

TCPServer::~TCPServer() {

}

TCPServer::init() {
        int sockfd;
        int clilen;
        struct sockaddr_in cli_addr, serv_addr;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            throw std::runtime_error("Error creating socket");

        memset(&cli_addr, 0, sizeof(cli_addr));
        memset(&serv_addr, 0, sizeof(cli_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_family =  INADDR_ANY;
        serv_addr.sin_port = htons(port_);

        LOG<INFO>("Created socket in TCPServer");

        int ret = bind(sockfd, static_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));
        if (ret < 0)
            throw std::runtime_error("Error binding in port " + to_string(port_));

        listen(sockfd, queue_len_);
        clilen = sizeof(cli_addr);
}
    
TCPServer::loop() {
    while (1) {
        int newsock = accept(sockfd, static_cast<struct sockaddr*>(&cli_addr), &clilen);
        if (newsock < 0)
            throw std::runtime_error("Error accepting socket");

        process(newsock);

        close(newsock);
    }
}

} // namespace sirius
