#include "src/server/TCPServer.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <vector>

#include "src/utils/logging.h"  // TODO: remove word src
#include "src/common/schemas/TCPBladeMessage_generated.h"

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

    while (1) {
        int newsock = accept(server_sock_,
                reinterpret_cast<struct sockaddr*>(&cli_addr), &clilen);
        if (newsock < 0)
            throw std::runtime_error("Error accepting socket");

        process(newsock);

        close(newsock);  // Do this for now, likely change later?
    }
}

void TCPServer::process(int sock) {
    LOG<INFO>("Processing socket: ", sock);
    char buffer[1024] = {0};
    int retval = read(sock, buffer, 1024);
    if (retval < 0) {
        printf("bad things happened \n");
    }
    std::vector<char> demo;
    demo.push_back('x');
    printf("%s\n", buffer);
    std::string hello = "hello this is the server";
    send(sock, hello.c_str(), hello.length(), 0);
}

}  // namespace cirrus
