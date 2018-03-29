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
#include <memory>
#include "ModelGradient.h"
#include "Utils.h"
#include "SparseLRModel.h"
#include "SparseMFModel.h"
#include "Model.h"

class PSSparseServerInterface {
 public:
  PSSparseServerInterface(const std::string& ip, int port);

  void send_lr_gradient(const LRSparseGradient&);
  void send_mf_gradient(const MFSparseGradient&);
  
  SparseLRModel get_lr_sparse_model(const SparseDataset& ds, const Configuration& config);
  void get_lr_sparse_model_inplace(const SparseDataset& ds, SparseLRModel&, const Configuration& config);
  SparseMFModel get_sparse_mf_model(const SparseDataset& ds, uint32_t, uint32_t);
  
  // XXX: Change this to fit with Joao's changes - Andy
  SparseLRModel get_full_model();
  void get_full_model(SparseLRModel& model, int server_index, int num_servers);
  SparseLRModel get_sparse_model(const SparseDataset& ds);
  void get_sparse_model(const SparseDataset& ds, int i, int n, SparseLRModel& model);
  void get_sparse_model(char* msg_begin, uint32_t num_weights, SparseLRModel& model, int server_index, int num_servers);
  // XXX: END - Andy


  std::unique_ptr<CirrusModel> get_full_model(bool isCollaborativeFiltering); //XXX use a better argument here

  void set_status(uint32_t id, uint32_t status);
  uint32_t get_status(uint32_t id);

 private:
  std::string ip;
  int port;
  int sock;
};

#endif //  PS_SPARSE_SERVER_INTERFACE_H_
