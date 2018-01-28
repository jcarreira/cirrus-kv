#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include "ModelGradient.h"
#include "Utils.h"
#include "SparseLRModel.h"

class PSSparseServerInterface {
 public:
  PSSparseServerInterface(const std::string& ip, int port);

  void send_gradient(const LRSparseGradient&);
  
  void get_model(SparseLRModel& model);
  SparseLRModel get_sparse_model(const SparseDataset& ds);

 private:
  std::string ip;
  int port;
  int sock;
};

PSSparseServerInterface::PSSparseServerInterface(const std::string& ip, int port) :
  ip(ip), port(port) {

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    throw std::runtime_error("Error when creating socket.");
  }   
  int opt = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt))) {
    throw std::runtime_error("Error setting socket options.");
  }   

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) != 1) {
    throw std::runtime_error("Address family invalid or invalid "
        "IP address passed in");
  }   
  // Save the port in the info
  serv_addr.sin_port = htons(port);
  std::memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));

  // Connect to the server
  if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    throw std::runtime_error(
        "Client could not connect to server."
        " Address: " + ip + " port: " + std::to_string(port));
  }
}

void PSSparseServerInterface::send_gradient(const LRSparseGradient& gradient) {
  uint32_t operation = 1;
  int ret = send(sock, &operation, sizeof(uint32_t), 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending operation");
  }
  //std::cout << "Sent ret: " << ret << std::endl;

  uint32_t size = gradient.getSerializedSize();
  //std::cout << "Sending grad size: " << size << std::endl;
  ret = send(sock, &size, sizeof(uint32_t), 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending grad size");
  }
  //std::cout << "Sent ret: " << ret << std::endl;
  char data[size];
  gradient.serialize(data);
  ret = send(sock, data, size, 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending grad");
  }
}

ssize_t read_all(int sock, void* data, size_t len) {
  uint64_t bytes_read = 0;

  while (bytes_read < len) {
    int64_t retval = read(sock, reinterpret_cast<char*>(data) + bytes_read,
        len - bytes_read);

    if (retval == -1) {
      throw std::runtime_error("Error reading from client");
    } else if (retval == 0) {
      // end of file
      throw std::runtime_error("End of file in socket");
    }

    bytes_read += retval;
  }   

  return bytes_read;
}

void PSSparseServerInterface::get_model(SparseLRModel& model) {
  (void)model;
  uint32_t operation = 2;
  int ret = send(sock, &operation, sizeof(uint32_t), 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending operation");
  }

  uint32_t size;
  ret = read_all(sock, &size, sizeof(size));
  std::cout << "Model with size: " << size << std::endl;

  char* data = new char[size];
  read_all(sock, data, size);
  model.loadSerialized(data);
  delete[] data;
}

#define MAX_MSG_SIZE (1024*1024)

SparseLRModel PSSparseServerInterface::get_sparse_model(const SparseDataset& ds) {
  // we don't know the number of weights to start with
  char* msg = new char[MAX_MSG_SIZE];
  char* msg_begin = msg; // need to keep this pointer to delete later

  uint32_t num_weights = 0;
  store_value<uint32_t>(msg, num_weights); // just make space for the number of weights
  for (const auto& sample : ds.data_) {
    for (const auto& w : sample) {
      store_value<uint32_t>(msg, w.first); // encode the index
      num_weights++;
      //assert(std::distance(msg_begin, msg) < MAX_MSG_SIZE);
    }
  }

  // put num_weights in the beginning

  // 1. Send operation
  uint32_t operation = 2;
  send_all(sock, &operation, sizeof(uint32_t));
  // 2. Send msg size
  uint32_t msg_size = sizeof(uint32_t) + sizeof(uint32_t) * num_weights;
  send_all(sock, &msg_size, sizeof(uint32_t));
  // 3. Send num_weights + weights
  msg = msg_begin;
  store_value<uint32_t>(msg, num_weights);
  std::cout << "Getting model. Sending weights msg size: " << msg_size << std::endl;
  send_all(sock, msg_begin, msg_size);
  delete[] msg_begin;

  //4. receive weights from PS
  uint32_t to_receive_size = sizeof(FEATURE_TYPE) * num_weights;
  std::cout << "Model sent. Receiving: " << num_weights << " weights" << std::endl;

  char* buffer = new char[to_receive_size];
  read_all(sock, buffer, to_receive_size);

  // build a truly sparse model and return
  SparseLRModel model(0);
  model.loadSerializedSparse(buffer, num_weights);
  
  delete[] buffer;

  return std::move(model);
}

