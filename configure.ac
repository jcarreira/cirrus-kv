#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(SIRIUS, 0.1, joao@berkeley.edu)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/server/RDMAServer.h])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AX_CXX_COMPILE_STDCXX_14

CXXFLAGS="-O3 -fPIC -std=c++1z"

# Checks for libraries.
#AC_CHECK_LIB([ibverbs], [ibv_create_qp])
#AC_CHECK_LIB([rdmacm], [rdma_get_cm_event])

AC_CHECK_LIB([ibverbs], [main])
AC_CHECK_LIB([rdmacm], [main])

# Checks for header files.
AC_CHECK_HEADERS([netdb.h stdint.h stdlib.h sys/socket.h \
        sys/time.h syslog.h unistd.h google/dense_hash_map])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRCOLL
AC_CHECK_FUNCS([gettimeofday localtime_r memchr memmove memset mkdir stpcpy strchr strcspn strdup strerror strpbrk strrchr strspn strstr])
AC_CONFIG_FILES([Makefile
                 examples/Makefile
                 src/Makefile
                 src/authentication/Makefile
                 src/utils/Makefile
                 src/common/Makefile
                 src/object_store/Makefile
                 src/client/Makefile
                 src/server/Makefile
                 src/test/Makefile])

AC_OUTPUT
