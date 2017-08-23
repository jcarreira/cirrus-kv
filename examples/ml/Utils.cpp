#include <Utils.h>
#include <time.h>

void check_mpi_error(int err, std::string error) {
    if (err != MPI_SUCCESS) {
        if (error == "") {
            throw std::runtime_error("Unknown error in MPI.");
        } else {
            throw std::runtime_error(error);
        }
    }
}

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

