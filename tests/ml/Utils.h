/* Copyright Joao Carreira 2017 */

#ifndef SRC_UTILS_UTILS_H_
#define SRC_UTILS_UTILS_H_

#include <mpi.h>
#include <sys/time.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <random>
#include <cfloat>

#define LOG2(X) ((unsigned) (8*sizeof (unsigned long long) - \
            __builtin_clzll((X)) - 1)

/**
  * Check output of an MPI return code
  * Throws exception with error message in case of MPI error
  */
void check_mpi_error(int err, std::string error = "");

/**
  * Get current time/epoch in microseconds
  */
uint64_t get_time_us();

/**
  * Get current time/epoch in milliseconds
  */
uint64_t get_time_ms();

/**
  * Convert string to arbitrary type
  */
template <class C>
C string_to(const std::string& s) {
    if (s == "") {
        return 0;
    } else {
        std::istringstream iss(s);
        C c;
        iss >> c;
        return c;
    }
}

/**
  * Convert arbitrary type to string
  */
template <class C>
std::string to_string(const C& s) {
    std::ostringstream oss;
    oss << s;
    return oss.str();
}

/**
  * Return the size of a file
  */
std::ifstream::pos_type filesize(const std::string& filename);

/**
  * Initializes mpi
  */
void init_mpi(int argc, char**argv);

/**
  * Returns the name of the host where the process is running
  */
std::string hostname();

/**
  * Get a random number
  */
unsigned int get_rand();

/**
  * Get a random number between 0 and 1
  */
double get_rand_between_0_1();

/**
  * Used to delete arrays of arbitrary teypes
  */
template<typename T>
void array_deleter(T* t) {
    delete[] t;
}

/**
  * Print statistical metrics of values that can be iterated on
  */
template <typename T>
void print_statistics(const T& begin, const T& end) {
    double max_val = DBL_MIN;
    double min_val = DBL_MAX;
    double avg = 0;

    for (T it = begin; it != end; ++it) {
        avg += *it;
        if (*it > max_val)
            max_val = *it;
        if (*it < min_val)
            min_val = *it;
    }

    std::cout << std::endl
        << "Min: " << min_val << std::endl
        << "Max: " << max_val << std::endl
        << "avg: " << avg / std::distance(begin, end) << std::endl
        << "distance: " << std::distance(begin, end) << std::endl
        << std::endl;
}

#endif  // SRC_UTILS_UTILS_H_
