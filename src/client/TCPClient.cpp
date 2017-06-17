#include "client/TCPClient.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>

namespace cirrus {

void TCPClient::connect(std::string address, std::string port_string) {
    // Create socket

    // TODO: Replace with logged errors and exit
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("socket creation error \n \n");
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));

    // Set the type of address being used, assuming ip v4
    serv_addr.sin_family = AF_INET:

    // Convert port from string to int
    int port = stoi(port_string, nullptr);

    // Save the port in the info
    serv_addr.sin_port = htons(port);

    // Connect to the server
    // TODO: Replace with logged errors and exit
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr) < 0) {
      printf("could not connect to server\n\n");
    }
}

void TCPClient::test() {
  char *hello = "Hello from client";
  send(sock , hello , strlen(hello) , 0);
}
}  // namespace cirrus
