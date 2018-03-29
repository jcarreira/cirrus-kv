#include <cassert>
#include "PSSparseServerInterface.h"

#undef DEBUG

#define MAX_MSG_SIZE (1024*1024)

PSSparseServerInterfaceWrapper::PSSparseServerInterfaceWrapper(const std::string& ip, int port, int num_servers) {
  for (int i = 0; i < 2; i++) { // replace 2 with num_servers
    psint[i] = new PSSparseServerInterface(ip, port + i);
  }
}

void PSSparseServerInterfaceWrapper::send_gradient(const LRSparseGradient* gradient) {
  // need to generalize to arbitrary num of servers 
  LRSparseGradient split_model[2] = gradient.shard(2);
  psint[0]->send_gradient(split_model[0]);
  psint[1]->send_gradient(split_model[1]);
}


SparseLRModel PSSparseServerInterface::get_sparse_model(const SparseDataset& ds) {
  SparseLRModel model(0);
  // we get the model subset with just the right amount of weights
  for (int i = 0; i < 2; i++) {
    psint[i]->get_sparse_model(*dataset, i, 2, model);
  }
  return model;
}


SparseLRModel PSSparseServerInterface::get_full_model() {
  // placeholder for now NOT CORRECT
  return psint[0].get_full_model();
}
