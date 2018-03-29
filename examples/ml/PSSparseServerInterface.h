#ifndef PS_SPARSE_SERVER_INTERFACE_H_
#define PS_SPARSE_SERVER_INTERFACE_H_

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

  //void get_model(SparseLRModel& model);
  SparseLRModel get_full_model();
  void get_full_model(SparseLRModel& model, int server_index, int num_servers);
  SparseLRModel get_sparse_model(const SparseDataset& ds);
  void get_sparse_model(char* msg_begin, uint32_t num_weights, SparseLRModel& model, int server_index, int num_servers);

 private:
  std::string ip;
  int port;
  int sock;
};

#endif //  PS_SPARSE_SERVER_INTERFACE_H_
