#include <examples/ml/Utils.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>

#if 0
void check_mpi_error(int err, std::string error) {
    if (err != MPI_SUCCESS) {
        if (error == "") {
            throw std::runtime_error("Unknown error in MPI.");
        } else {
            throw std::runtime_error(error);
        }
    }
}
#endif

uint64_t get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000000UL * tv.tv_sec + tv.tv_usec;
}

uint64_t get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (1000000UL * tv.tv_sec + tv.tv_usec) / 1000;
}

uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_nsec + ts.tv_sec * 1000000000UL;
}

std::ifstream::pos_type filesize(const std::string& filename) {
    std::ifstream in(filename.c_str(),
            std::ifstream::ate | std::ifstream::binary);

    if (!in)
        throw std::runtime_error("Error opening: " + filename);

    return in.tellg();
}

#if 0
void init_mpi(int argc, char**argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        std::cerr
            << "MPI implementation does not support multiple threads"
            << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}
#endif

std::string hostname() {
    char name[200] = {0};
    gethostname(name, sizeof(name));
    return name;
}

unsigned int get_rand() {
    static std::mt19937 mt_rand(42);

    auto res = mt_rand();

    return res;
}

double get_rand_between_0_1() {
    static std::mt19937 mt_rand(42);

    return 1.0 * mt_rand() / mt_rand.max();
}

double get_random_normal(double mean, double var) {
  static std::default_random_engine generator;
  std::normal_distribution<double> distribution(mean, std::sqrt(var));

  return distribution(generator);
}

void sleep_forever() {
    while (1) {
        sleep(1000);
    }
}

int64_t send_all(int sock, void* data, size_t len) {
  uint64_t bytes_sent = 0;

  while (bytes_sent < len) {
    int64_t retval = send(sock, reinterpret_cast<char*>(data) + bytes_sent,
        len - bytes_sent, 0);

    if (retval == -1) {
      return -1;
    }

    bytes_sent += retval;
  }   

  return bytes_sent;
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
