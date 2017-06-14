#include "src/server/EPollTCPServer.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include "src/utils/logging.h"

namespace cirrus {

EPollTCPServer::EPollTCPServer(int port, int queue_len) {
    port_ = port;
    queue_len_ = queue_len;
    max_conn_ = 1000;
    listener = 0;
}

EPollTCPServer::~EPollTCPServer() {
}

void EPollTCPServer::make_non_blocking(int sock) {
    int flags, s;

    flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        std::runtime_error("Error in F_GETFL");
    }

    flags |= O_NONBLOCK;
    s = fcntl(sock, F_SETFL, flags);
    if (s == -1) {
        std::runtime_error("Error in F_SETFL");
    }
}

void EPollTCPServer::init() {
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        throw std::runtime_error("Error creating socket");
    }

    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port_);

    memset(&serveraddr, 0, sizeof(serveraddr));
    if (bind(listener, reinterpret_cast<sockaddr*>(&serveraddr),
                sizeof(serveraddr)) == -1) {
        throw std::runtime_error(
                std::string("Error binding socket into port ") +
                to_string(port_));
    }

    make_non_blocking(listener);

    if (listen(listener, queue_len_) == -1) {
        throw std::runtime_error("Error listening");
    }

    fdmax_ = listener;

    events = new epoll_event[max_conn_];

    epfd_ = epoll_create(max_conn_);
    if (epfd_ < 0) {
        throw std::runtime_error("Error creating epoll");
    }

    ev.events = EPOLLIN;
    ev.data.fd = fdmax_;

    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fdmax_, &ev) < 0) {
        throw std::runtime_error("Error in epoll_ctl");
    }
}

void EPollTCPServer::loop() {
    int index = 0;  // ??
    struct sockaddr_in clientaddr;

    while (1) {
        int res = epoll_wait(epfd_, events, max_conn_, -1);
        if (res == -1) {
            throw std::runtime_error("Error in epoll_wait");
        }
        int client_fd = events[index].data.fd;

        for (index = 0; index < max_conn_; index++) {
            if (client_fd == listener) {  // new connection ??
                socklen_t addrlen = sizeof(clientaddr);
                int newfd = accept(listener,
                        reinterpret_cast<sockaddr*>(&clientaddr), &addrlen);

                if (newfd < 0) {
                    throw std::runtime_error("Error accepting new connection");
                } else {
                    LOG<INFO>("Accepted new connection at socket ", newfd);
                    ev.events = EPOLLIN;
                    ev.data.fd = newfd;
                    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, newfd, &ev) < 0) {
                        throw std::runtime_error("Error epoll_ctl");
                    }
                }
                break;
            } else {
                if (events[index].events & EPOLLHUP) {
                    if (epoll_ctl(epfd_, EPOLL_CTL_DEL, client_fd, &ev) < 0) {
                        throw std::runtime_error("Error in EPOLLHUP");
                    }
                    close(client_fd);
                    break;
                } else if (events[index].events & EPOLLIN) {
                    process(client_fd);
                }
            }
        }
    }
}

void EPollTCPServer::process(int socket) {
    LOG<INFO>("Processing request from socket: ", socket);
}

}  // namespace cirrus
