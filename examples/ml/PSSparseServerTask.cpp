#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "async.h"

#undef DEBUG

#define APPLY_GRADIENT_REQ (1)
#define GET_MODEL_REQ (2)
#define GET_FULL_MODEL_REQ (3)

#define MAX_CONNECTIONS (201) // (2 x # workers + 1)
    
static void update_model(auto&);

namespace PSSparseServerTaskGlobal {
  // used to monitor how much since last received gradient
  static auto prev_on_msg_time = get_time_us();

  static Configuration config;
  static std::mutex mp_start_lock; // used as barrier
 
  static std::unique_ptr<SparseLRModel> model; // last computed model
  std::mutex model_lock; // used to coordinate access to the last computed model

  redisContext* redis_con;
  volatile uint64_t onMessageCount = 0;
}

auto PSSparseServerTask::connect_redis() {
  auto redis_con  = redis_connect(REDIS_IP, REDIS_PORT);
  if (redis_con == NULL || redis_con -> err) {
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
  return redis_con;
}

PSSparseServerTask::PSSparseServerTask(const std::string& redis_ip, uint64_t redis_port,
    uint64_t MODEL_GRAD_SIZE, uint64_t MODEL_BASE,
    uint64_t LABEL_BASE, uint64_t GRADIENT_BASE,
    uint64_t SAMPLE_BASE, uint64_t START_BASE,
    uint64_t batch_size, uint64_t samples_per_batch,
    uint64_t features_per_sample, uint64_t nworkers,
    uint64_t worker_id) :
  MLTask(redis_ip, redis_port, MODEL_GRAD_SIZE, MODEL_BASE,
      LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
      batch_size, samples_per_batch, features_per_sample,
      nworkers, worker_id) {
  gradientVersions.resize(nworkers, 0);

  std::cout << "PSSparseServerTask is built" << std::endl;
}

std::shared_ptr<char> PSSparseServerTask::serialize_model(const SparseLRModel& model, uint64_t* model_size) {
  *model_size = model.getSerializedSize();
  auto d = std::shared_ptr<char>(
      new char[*model_size], std::default_delete<char[]>());
  model.serializeTo(d.get());
  return d;
}

bool PSSparseServerTask::testRemove(struct pollfd x) {
  // If this pollfd will be removed, the index of the next location to insert
  // should be reduced by one correspondingly.
  if (x.fd == -1) {
    curr_index -= 1;
  }
  return x.fd == -1;
}

std::mutex to_process_lock;
sem_t sem_new_req;
struct Request {
  public:
    Request(int req_id, int sock, std::vector<char>&& vec, struct pollfd& poll_fd) :
      req_id(req_id), sock(sock), vec(std::move(vec)), poll_fd(poll_fd){}

  int req_id;
  int sock;
  std::vector<char> vec;
  struct pollfd& poll_fd;
};
std::queue<Request> to_process;

void PSSparseServerTask::gradient_f() {
  std::vector<char> thread_buffer;
  thread_buffer.resize(1024 * 1024); // 1MB
  while (1) {
    sem_wait(&sem_new_req);
    to_process_lock.lock();
    Request req = std::move(to_process.front());
    to_process.pop();
    to_process_lock.unlock();

#ifdef DEBUG
    std::cout << "Processing request: " << req.req_id << std::endl;
#endif

    if (req.req_id == APPLY_GRADIENT_REQ) {
      std::vector<char>& buffer = req.vec;
      uint32_t* incoming_size_ptr = reinterpret_cast<uint32_t*>(buffer.data());
      uint32_t incoming_size = *incoming_size_ptr;
#ifdef DEBUG 
      std::cout << "incoming size: " << incoming_size << std::endl;
#endif
      if (incoming_size > thread_buffer.size()) {
        throw std::runtime_error("Not enough buffer");
      }
      //buffer.resize(incoming_size);
      try {
        read_all(req.sock, thread_buffer.data(), incoming_size);
      } catch(...) {
        throw std::runtime_error("Uhandled error");
      }

      LRSparseGradient gradient(0);
      gradient.loadSerialized(thread_buffer.data());

      PSSparseServerTaskGlobal::model_lock.lock();
      PSSparseServerTaskGlobal::model->sgd_update_adagrad(
          PSSparseServerTaskGlobal::config.get_learning_rate(), &gradient);
      PSSparseServerTaskGlobal::model_lock.unlock();
      PSSparseServerTaskGlobal::onMessageCount++;
    } else if (req.req_id == GET_MODEL_REQ) {
      // need to parse the buffer to get the indices of the model we want 
      // to send back to the client
      std::vector<char>& buffer = req.vec;

      uint32_t* incoming_size_ptr = reinterpret_cast<uint32_t*>(buffer.data());
      uint32_t incoming_size = *incoming_size_ptr;
      if (incoming_size > thread_buffer.size()) {
        throw std::runtime_error("Not enough buffer");
      }
#ifdef DEBUG 
      std::cout << "incoming size: " << incoming_size << std::endl;
#endif
      //buffer.resize(incoming_size);
      try {
        read_all(req.sock, thread_buffer.data(), incoming_size);
      } catch(...) {
        throw std::runtime_error("Uhandled error");
      }

      const char* data = thread_buffer.data();
      uint64_t num_entries = load_value<uint32_t>(data);

      uint32_t to_send_size = num_entries * sizeof(FEATURE_TYPE);
      auto data_to_send = std::shared_ptr<char>(
          new char[to_send_size], std::default_delete<char[]>());
      char* data_to_send_ptr = data_to_send.get();
#ifdef DEBUG
      std::cout << "Sending back: " << num_entries
        << " weights from model. Size: " << to_send_size
        << std::endl;
#endif
      for (uint32_t i = 0; i < num_entries; ++i) {
        uint32_t entry_index = load_value<uint32_t>(data);
//#ifdef DEBUG
//        std::cout << entry_index << " ";
//#endif
        store_value<FEATURE_TYPE>(
            data_to_send_ptr,
            PSSparseServerTaskGlobal::model->get_nth_weight(entry_index));
      }
//#ifdef DEBUG
//      std::cout << std::endl;
//#endif
      send_all(req.sock, data_to_send.get(), to_send_size);
    } else if (req.req_id == GET_FULL_MODEL_REQ) {
      PSSparseServerTaskGlobal::model_lock.lock();
      auto model_copy = *PSSparseServerTaskGlobal::model;
      PSSparseServerTaskGlobal::model_lock.unlock();
      uint32_t model_size = model_copy.getSerializedSize();

      auto d = std::shared_ptr<char>(
          new char[model_size], std::default_delete<char[]>());
      model_copy.serializeTo(d.get());
      //std::cout << "Sending model message size: " << model_size << std::endl;
      send_all(req.sock, d.get(), model_size);
    } else {
      throw std::runtime_error("gradient_f: Unknown operation");
    }

    req.poll_fd.events = POLLIN; // XXX explain this
    //kill(getpid(), SIGCONT);
    pthread_kill(poll_thread, SIGCONT);
    //pthread_kill(poll_thread, SIGUSR1);
//    to_process_lock.lock();
//    to_process.pop();
//    to_process_lock.unlock();
#ifdef DEBUG
    std::cout << "gradient_f done" << std::endl;
#endif
  }
}

/**
  * FORMAT
  * operation (uint32_t)
  * incoming size (uint32_t)
  * buffer with previous size
  */
bool PSSparseServerTask::process(struct pollfd& poll_fd) {
  int sock = poll_fd.fd;
#ifdef DEBUG
  std::cout << "Processing socket: " <<  sock << std::endl;
#endif
  std::vector<char> buffer;
  uint64_t current_buf_size = sizeof(uint32_t);
  buffer.reserve(current_buf_size);
  
  uint64_t bytes_read = 0;
  bool ret = read_from_client(buffer, sock, bytes_read);
  if (!ret) {
    return false;
  }

  uint32_t* operation_ptr = reinterpret_cast<uint32_t*>(buffer.data());
  uint32_t operation = *operation_ptr;
 
#ifdef DEBUG 
  std::cout << "Operation: " << operation << std::endl;
#endif

  if (operation == APPLY_GRADIENT_REQ || operation == GET_MODEL_REQ) { // GRADIENT
    uint64_t bytes_read = 0;
    bool ret = read_from_client(buffer, sock, bytes_read);
    if (!ret) {
      return false;
    }

#if 0
    uint32_t* incoming_size_ptr = reinterpret_cast<uint32_t*>(buffer.data());
    uint32_t incoming_size = *incoming_size_ptr;
#ifdef DEBUG 
    std::cout << "incoming size: " << incoming_size << std::endl;
#endif
    if (incoming_size > current_buf_size) {
      buffer.resize(incoming_size);
    }

    try {
      read_all(sock, buffer.data(), incoming_size);
    } catch(...) {
      return false;
    }
#endif

    to_process_lock.lock();
    poll_fd.events = 0; // explain this
    to_process.push(Request(operation, sock, std::move(buffer), poll_fd));
    to_process_lock.unlock();
    sem_post(&sem_new_req);
  } else if (operation == GET_FULL_MODEL_REQ) {
    to_process_lock.lock();
    poll_fd.events = 0; //XXX explain this
    to_process.push(Request(operation, sock, std::vector<char>(), poll_fd));
    to_process_lock.unlock();
    sem_post(&sem_new_req);
  } else {
    throw std::runtime_error("process: Unknown operation");
  }
  return true;
}

bool PSSparseServerTask::read_from_client(
    std::vector<char>& buffer, int sock, uint64_t& bytes_read) {
  bool first_loop = true;
  while (bytes_read < static_cast<int>(sizeof(uint32_t))) {
    int retval = read(sock, buffer.data() + bytes_read,
        sizeof(uint32_t) - bytes_read);

    if (first_loop && retval == 0) {
      // Socket is closed by client if 0 bytes are available
      close(sock);
      return false;
    }
    if (retval < 0) {
      close(sock);
      return false; // likely because lambda worker died after 5 minutes
      throw std::runtime_error("Server issue in reading socket during size read.");
    }
    bytes_read += retval;
    first_loop = false;
  }
  return true;
}

void PSSparseServerTask::start_server() {
  PSSparseServerTaskGlobal::model.reset(new SparseLRModel(MODEL_GRAD_SIZE));
  PSSparseServerTaskGlobal::model->randomize();
  
  sem_init(&sem_new_req, 0, 0);

  server_thread = std::make_unique<std::thread>(
      std::bind(&PSSparseServerTask::start_server2, this));

  for (uint32_t i = 0; i < n_threads; ++i) {
    gradient_thread.push_back(std::make_unique<std::thread>(
        std::bind(&PSSparseServerTask::gradient_f, this)));
  }
}

void PSSparseServerTask::start_server2() {
  std::cout << "Starting server2" << std::endl;

  poll_thread = pthread_self();

  server_sock_ = socket(AF_INET, SOCK_STREAM, 0); 
  if (server_sock_ < 0) {
    throw cirrus::ConnectionException("Server error creating socket");
  }   

  int opt = 1;
  if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    throw std::runtime_error("Error forcing port binding");
  }
  if (setsockopt(server_sock_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt))) {
    throw std::runtime_error("Error setting socket options.");
  }   

  if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
    throw std::runtime_error("Error forcing port binding");
  }   

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port_);
  std::memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));

  int ret = bind(server_sock_, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));
  if (ret < 0) {
    throw std::runtime_error("Error binding in port " + to_string(port_));
  }

  if (listen(server_sock_, SOMAXCONN) == -1) {
    throw std::runtime_error("Error listening on port " + to_string(port_));
  }
  fds.at(curr_index).fd = server_sock_;
  fds.at(curr_index).events = POLLIN;
  curr_index++;
  loop();
}

uint32_t num_connections = 0;

void PSSparseServerTask::loop() {
  struct sockaddr_in cli_addr;
  socklen_t clilen = sizeof(cli_addr);

  std::cout << "Starting loop" << std::endl;
  while (1) {
    int poll_status = poll(fds.data(), curr_index, timeout);
    if (poll_status == -1) {
      if (errno != EINTR) {
        throw std::runtime_error("Server error calling poll.");
      } else {
        //std::cout << "EINTR" << std::endl;
      }
    } else if (poll_status == 0) {
      //std::cout << timeout << " ms elapsed" << std::endl;
    } else {
      // there is at least one pending event, find it.
      for (uint64_t i = 0; i < curr_index; i++) {
	struct pollfd& curr_fd = fds.at(i);
	// Ignore the fd if we've said we don't care about it
	if (curr_fd.fd == -1) {
	  continue;
	}
	if (curr_fd.revents != POLLIN) {
	  //LOG<ERROR>("Non read event on socket: ", curr_fd.fd);
	  if (curr_fd.revents & POLLHUP) {
            std::cout << "PS closing connection " << num_connections << std::endl;
            num_connections--;
	    //LOG<INFO>("Connection was closed by client");
	    //LOG<INFO>("Closing socket: ", curr_fd.fd);
	    close(curr_fd.fd);
	    curr_fd.fd = -1;
	  }
	} else if (curr_fd.fd == server_sock_) {
          std::cout << "PS new connection!" << std::endl;
	  int newsock = accept(server_sock_,
	      reinterpret_cast<struct sockaddr*>(&cli_addr),
	      &clilen);
	  if (newsock < 0) {
	    throw std::runtime_error("Error accepting socket");
	  }
	  // If at capacity, reject connection
          if (num_connections > (MAX_CONNECTIONS - 1)) {
            std::cout << "Rejecting connection "
              << num_connections
              << std::endl;
            close(newsock);
          } else if (curr_index == max_fds) {
            throw std::runtime_error("We reached capacity");
	    close(newsock);
	  } else {
	    //LOG<INFO>("Created new socket: ", newsock);
	    fds.at(curr_index).fd = newsock;
	    fds.at(curr_index).events = POLLIN;
	    curr_index++;
            num_connections++;
	  }
	} else {
#ifdef DEBUG
          std::cout << "Calling process" << std::endl;
#endif
	  if (!process(curr_fd)) {
            num_connections--;
            std::cout << "PS closing connection " << num_connections << std::endl;
	    //LOG<INFO>("Processing failed on socket: ", curr_fd.fd);
	    // do not make future alerts on this fd
	    curr_fd.fd = -1;
	  }
	}
	curr_fd.revents = 0;  // Reset the event flags
      }
    }
    // If at max capacity, try to make room
    if (curr_index == max_fds) {
      // Try to purge unused fds, those with fd == -1
      std::cout << "Purging" << std::endl;
      std::remove_if(fds.begin(), fds.end(),
          std::bind(&PSSparseServerTask::testRemove, this, std::placeholders::_1));
    }
  }
}
  
void sig_handler(int) {
  //std::cout << "Sig handler" << std::endl;
}

/**
  * This is the task that runs the parameter server
  * This task is responsible for
  * 1) sending the model to the workers
  * 2) receiving the gradient updates from the workers
  *
  */
void PSSparseServerTask::run(const Configuration& config) {
  std::cout
    << "PS task initializing model"
    << " MODEL_GRAD_SIZE: " << MODEL_GRAD_SIZE
    << std::endl;


  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    throw std::runtime_error("Unable to set signal handler");
  }
  start_server();

  // initialize model
  PSSparseServerTaskGlobal::model.reset(new SparseLRModel(MODEL_GRAD_SIZE));
  PSSparseServerTaskGlobal::model->randomize();
  std::cout << "[PS] "
    << "PS publishing model at id: " << MODEL_BASE
    << " csum: " << PSSparseServerTaskGlobal::model->checksum() << std::endl;

  PSSparseServerTaskGlobal::config = config;

  PSSparseServerTaskGlobal::redis_con = connect_redis();
  //publish_model_redis();
  wait_for_start(PS_SPARSE_SERVER_TASK_RANK, PSSparseServerTaskGlobal::redis_con, nworkers);

  uint64_t start = get_time_us();
  uint64_t last_tick = get_time_us();
  while (1) {
    auto now = get_time_us();
    auto elapsed_us = now - last_tick;
    auto since_start_sec = 1.0 * (now - start) / 1000000;
    if (elapsed_us > 1000000) {
      last_tick = now;
      std::cout << "Events in the last sec: " 
        << 1.0 * PSSparseServerTaskGlobal::onMessageCount / elapsed_us * 1000 * 1000
        << " since (sec): " << since_start_sec
        << " #conns: " << num_connections
        << std::endl;
      PSSparseServerTaskGlobal::onMessageCount = 0;
    }
    sleep(1);
  }
}

void PSSparseServerTask::publish_model_redis() {
#ifdef DEBUG
  static int publish_count = 0;
  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << " publish_count: " << (++publish_count)
    << "\n";
#endif

  PSSparseServerTaskGlobal::model_lock.lock();
  auto model_copy = *PSSparseServerTaskGlobal::model;
  PSSparseServerTaskGlobal::model_lock.unlock();

#ifdef DEBUG
  std::cout << "Checking model" << std::endl;
  model_copy.check();
#endif
  
  uint64_t model_size;
  std::shared_ptr<char> data = serialize_model(model_copy, &model_size);

#ifdef DEBUG
  std::cout << "redisCommand PUBLISH MODEL" << std::endl;
  auto before_us = get_time_us();
#endif

  //redis_put_binary_numid(PSSparseServerTaskGlobal::redis_con, MODEL_BASE,
  //    reinterpret_cast<const char*>(data.get()), model_size);
#ifdef DEBUG
  auto elapsed_us = get_time_us() - before_us;
  std::cout 
    << "Put model elapsed (us): " << elapsed_us
    << " bw (MB/s): " << (1.0 * model_size / 1024 / 1024 / elapsed_us * 1000 * 1000)
    << "\n";
#endif
}

