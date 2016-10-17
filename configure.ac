#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(DDC, 0.1, joao@berkeley.edu)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/server/RDMAServer.h])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_LANG(C++)
AC_PROG_CXX
AC_PROG_CC

CXXFLAGS="-std=c++11 -fPIC"

# Checks for libraries.
AC_CHECK_LIB([ibverbs], [ibv_create_qp])
AC_CHECK_LIB([rdmacm], [rdma_get_cm_event])

# Checks for header files.
AC_CHECK_HEADERS([netdb.h stdint.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday localtime_r memset mkdir strerror strstr])
AC_CONFIG_FILES([Makefile
                 examples/Makefile
                 src/Makefile
                 src/client/Makefile
                 src/resource_allocator/Makefile
                 src/server/Makefile
                 src/utils/Makefile])

AC_OUTPUT
