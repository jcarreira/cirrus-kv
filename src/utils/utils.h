#ifndef SRC_UTILS_UTILS_H_
#define SRC_UTILS_UTILS_H_

#include <errno.h>
#include <cstdio>

#define DIE(s) { \
    fprintf(stderr, s);\
    fflush(stderr); \
    std::cout << "Exiting" << std::endl; \
    exit(-1);\
}

// die if not zero
#define TEST_NZ(x) do { \
    if ((x)) { \
        printf("errno: %d\n", errno); \
        DIE("error: " #x " failed (returned non-zero).");}\
} while (0)

// template<typename T>
// void TEST_NZ(const T& t) {
//    if (t) {
//        DIE("error: " #x " failed (returned non-zero).");
//    }
//}

// die if zero
#define TEST_Z(x)  do { \
    if (!(x)) { \
        printf("errno: %d\n", errno); \
        DIE("error: " #x " failed (returned zero/null).");}\
} while (0)

#endif  // SRC_UTILS_UTILS_H_
