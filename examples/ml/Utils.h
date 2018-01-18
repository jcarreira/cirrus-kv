#ifndef EXAMPLES_ML_UTILS_H_
#define EXAMPLES_ML_UTILS_H_

#include <sys/time.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <random>
#include <cfloat>

#define LOG2(X) ((unsigned) (8*sizeof (uint64_t) - \
            __builtin_clzll((X)) - 1)

#define FLOAT_EPS (1e-7)
#define FLOAT_EQ(A, B) (std::fabs((A) - (B)) < FLOAT_EPS)

/**
  * Get current time/epoch in ns
  */
uint64_t get_time_ns();

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

void sleep_forever();

template<typename T>
void store_value(char*& data, T value) {
  T* v_ptr = reinterpret_cast<T*>(data);
  *v_ptr = value;
  data += sizeof(T);
}

template<typename T>
T load_value(const char*& data) {
  const T* v_ptr = reinterpret_cast<const T*>(data);
  T ret = *v_ptr;
  data += sizeof(T);
  return ret;
}


#endif  // EXAMPLES_ML_UTILS_H_
