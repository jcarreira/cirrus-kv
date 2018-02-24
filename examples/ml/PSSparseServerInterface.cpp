#include "PSSparseServerInterface.h"
#include "Constants.h"

//#define DEBUG

#define MAX_MSG_SIZE (1024*1024)

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
  uint32_t operation = SEND_GRADIENT;
  int ret = send(sock, &operation, sizeof(uint32_t), 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending operation");
  }

  uint32_t size = gradient.getSerializedSize();
  ret = send(sock, &size, sizeof(uint32_t), 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending grad size");
  }
  
  char data[size];
  gradient.serialize(data);
  ret = send(sock, data, size, 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending grad");
  }
}

void PSSparseServerInterface::send_mf_gradient(const MFSparseGradient& gradient) {
  uint32_t operation = SEND_MF_GRADIENT;
  int ret = send(sock, &operation, sizeof(uint32_t), 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending operation");
  }

  uint32_t size = gradient.getSerializedSize();
  ret = send(sock, &size, sizeof(uint32_t), 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending grad size");
  }
  
  char data[size];
  gradient.serialize(data);
  ret = send(sock, data, size, 0);
  if (ret == -1) {
    throw std::runtime_error("Error sending grad");
  }
}

/**
  * This function needs to send to the PS a list of users and items
  * FORMAT of message to send is:
  * K item ids to send (uint32_t)
  * base user id (uint32_t)
  * minibatch size (uint32_t)
  * list of K item ids (K * uint32_t)
  */
SparseMFModel PSSparseServerInterface::get_sparse_mf_model(
    const SparseDataset& ds, uint32_t user_base, uint32_t minibatch_size) {
  char* msg = new char[MAX_MSG_SIZE];
  char* msg_begin = msg; // need to keep this pointer to delete later
 
  std::vector<uint32_t> item_ids;
  store_value<uint32_t>(msg, 0); // we will write this value later
  store_value<uint32_t>(msg, user_base);
  store_value<uint32_t>(msg, minibatch_size);
  for (const auto& sample : ds.data_) {
    for (const auto& w : sample) {
      int movieId = w.first;
      store_value<uint32_t>(msg, movieId); // encode the index
    }
  }
  msg = msg_begin;
  store_value<uint32_t>(msg, item_ids.size()); // store correct value here
  
  // 1. Send operation
  uint32_t operation = GET_MF_SPARSE_MODEL;
  send_all(sock, &operation, sizeof(uint32_t));
  // 2. Send msg size
  uint32_t msg_size = sizeof(uint32_t) * 3 + sizeof(uint32_t) * item_ids.size();
  send_all(sock, &msg_size, sizeof(uint32_t));
  // 3. Send request message
  send_all(sock, msg_begin, msg_size);
  
  // 4. receive user vectors and item vectors
  // FORMAT here is
  // minibatch_size * user vectors. Each vector is user_id + user_bias + NUM_FACTORS * FEATURE_TYPE
  // num_item_ids * item vectors. Each vector is item_id + item_bias + NUM_FACTORS * FEATURE_TYPE
  uint32_t to_receive_size =
    sizeof(uint32_t) * 2 + // user_id and item_id
    (minibatch_size + item_ids.size() + 2) * sizeof(FEATURE_TYPE); // user/item bias + user/item vectors
  std::cout << "Request sent. Receiving: " << to_receive_size << " bytes" << std::endl;

  char* buffer = new char[to_receive_size];
  read_all(sock, buffer, to_receive_size);

  // build a sparse model and return
  SparseMFModel model((FEATURE_TYPE*)buffer, minibatch_size, item_ids.size());
  
  delete[] msg_begin;
  delete[] buffer;

  return std::move(model);
}

SparseLRModel PSSparseServerInterface::get_lr_sparse_model(const SparseDataset& ds) {
  // we don't know the number of weights to start with
  char* msg = new char[MAX_MSG_SIZE];
  char* msg_begin = msg; // need to keep this pointer to delete later

  uint32_t num_weights = 0;
  store_value<uint32_t>(msg, num_weights); // just make space for the number of weights
  for (const auto& sample : ds.data_) {
    for (const auto& w : sample) {
      store_value<uint32_t>(msg, w.first); // encode the index
      num_weights++;
    }
  }
  msg = msg_begin;
  store_value<uint32_t>(msg, num_weights); // store correct value here
#ifdef DEBUG
  assert(std::distance(msg_begin, msg) < MAX_MSG_SIZE);
  std::cout << std::endl;
#endif

  // 1. Send operation
  uint32_t operation = GET_LR_SPARSE_MODEL;
  send_all(sock, &operation, sizeof(uint32_t));
  // 2. Send msg size
  uint32_t msg_size = sizeof(uint32_t) + sizeof(uint32_t) * num_weights;
  send_all(sock, &msg_size, sizeof(uint32_t));
  // 3. Send num_weights + weights
  send_all(sock, msg_begin, msg_size);
  
  //4. receive weights from PS
  uint32_t to_receive_size = sizeof(FEATURE_TYPE) * num_weights;
  //std::cout << "Model sent. Receiving: " << num_weights << " weights" << std::endl;

  char* buffer = new char[to_receive_size];
  read_all(sock, buffer, to_receive_size); //XXX this takes 2ms once every 5 runs

  // build a truly sparse model and return
  SparseLRModel model(0);
  model.loadSerializedSparse((FEATURE_TYPE*)buffer, (uint32_t*)msg, num_weights);
  
  delete[] msg_begin;
  delete[] buffer;

  return std::move(model);
}

SparseLRModel PSSparseServerInterface::get_full_model() {
  // 1. Send operation
  uint32_t operation = GET_LR_FULL_MODEL;
  send_all(sock, &operation, sizeof(uint32_t));
  //2. receive size from PS
  int model_size;
  read_all(sock, &model_size, sizeof(int));
  char* model_data = new char[sizeof(int) + model_size * sizeof(FEATURE_TYPE)];
  char*model_data_ptr = model_data;
  store_value<int>(model_data_ptr, model_size);

  read_all(sock, model_data_ptr, model_size * sizeof(FEATURE_TYPE));

  SparseLRModel model(0);
  model.loadSerialized(model_data);
  
  delete[] model_data;

  return std::move(model);
}

