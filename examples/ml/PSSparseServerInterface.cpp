#include "PSSparseServerInterface.h"
#include <cassert>

#undef DEBUG

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
#ifdef DEBUG
  std::cout << "Sending gradient" << std::endl;
#endif
  uint32_t operation = APPLY_GRADIENT_REQ;
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

uint64_t PSSparseServerInterface::get_ps_clock() const {
  uint32_t operation = GET_SERVER_CLOCK_REQ;
  send_all(sock, &operation, sizeof(uint32_t));
  uint64_t server_clock;
  read_all(sock, &server_clock, sizeof(server_clock));
  return server_clock;
}

SparseLRModel PSSparseServerInterface::get_sparse_model(const SparseDataset& ds) {
#ifdef DEBUG
  std::cout << "Getting sparse model" << std::endl;
#endif
  while (1) {
    // get server clock
    uint64_t server_clock = get_ps_clock();
    // if we are further away from slowest worker we wait a bit and try again
    if (worker_clock > server_clock + STALE_THRESHOLD) {
      usleep(50);
    } else {
      return get_sparse_model_aux(ds);
    }
  }
}

SparseLRModel PSSparseServerInterface::get_sparse_model_aux(const SparseDataset& ds) {
#ifdef DEBUG
  std::cout << "Getting sparse model" << std::endl;
#endif
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
#ifdef DEBUG
  assert(std::distance(msg_begin, msg) < MAX_MSG_SIZE);
  std::cout << std::endl;
#endif

  // put num_weights in the beginning

  // 1. Send operation
  uint32_t operation = GET_MODEL_REQ;
  send_all(sock, &operation, sizeof(uint32_t));
  // 2. Send msg size
  uint32_t msg_size = sizeof(uint32_t) + sizeof(uint32_t) * num_weights;
  send_all(sock, &msg_size, sizeof(uint32_t));
  // 3. Send num_weights + weights
  msg = msg_begin;
  store_value<uint32_t>(msg, num_weights);
  //std::cout << "Getting model. Sending weights msg size: " << msg_size << std::endl;
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
  uint32_t operation = GET_FULL_MODEL_REQ;
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

/**
  * Used to inform the PS that this connection refers to a worker
  * used for the PS to maintain information on each worker (e.g., clock)
  */
void PSSparseServerInterface::register_worker() {
  std::cout << "Registering worker" << std::endl;
  uint32_t operation = REGISTER_WORKER_REQ;
  send_all(sock, &operation, sizeof(uint32_t));
}

