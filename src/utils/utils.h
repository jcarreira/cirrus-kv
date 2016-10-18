/* Copyright 2016 Joao Carreira */

#ifndef _UTILS_H_
#define _UTILS_H_

#define DIE(s) { \
    fprintf(stderr, s);\
    fflush(stderr); \
    exit(-1);\
}

// die if not zero
#define TEST_NZ(x) do { if ( (x)) DIE("error: " #x " failed (returned non-zero)." ); } while (0)

// die if zero
#define TEST_Z(x)  do { if (!(x)) DIE("error: " #x " failed (returned zero/null)."); } while (0)

#endif // _UTILS_H_
