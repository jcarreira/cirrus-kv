include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

DEFAULT_LIB = -L. -lcoordinator \
              -L../authentication/ -lauthentication \
              -L../utils/ -lutils \
              -L../common/ -lcommon $(LIBRDMACM) $(LIBIBVERBS)

noinst_LIBRARIES = libcoordinator.a
bin_PROGRAMS = coordinatormain

libcoordinator_a_SOURCES = Coordinator.cpp
libcoordinator_a_CPPFLAGS = -ggdb -I$(top_srcdir) \
                            -I$(top_srcdir)/third_party/flatbuffers/include \
                            -I$(top_srcdir)/src

LDFLAGS  = -pthread
LDADD    = $(DEFAULT_LIB)
CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src

coordinatormain_DEPENDENCIES = libcoordinator.a
coordinatormain_SOURCES  = CoordinatorMain.cpp
