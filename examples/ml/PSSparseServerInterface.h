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
  std::unique_ptr<CirrusModel> get_full_model(bool isCollaborativeFiltering); //XXX use a better argument here

  // get full model with server_index
  std::unique_ptr<CirrusModel> get_full_model(bool isCollaborative, int server_index, int num_ps, std::unique_ptr<CirrusModel> model);
  void get_lr_sparse_model_inplace_sharded(SparseLRModel& model, const Configuration& config, char* msg_begin, uint32_t num_weights, uint32_t server_index);
  void get_mf_sparse_model_inplace_sharded(SparseMFModel& model, const Configuration& config, char* msg_begin, uint32_t num_items, uint32_t server_index, uint64_t minibatch_size);

  void set_status(uint32_t id, uint32_t status);
  uint32_t get_status(uint32_t id);

 private:
  std::string ip;
  int port;
  int sock;
};

#endif //  PS_SPARSE_SERVER_INTERFACE_H_
