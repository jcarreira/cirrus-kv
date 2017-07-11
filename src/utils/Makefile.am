include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libutils.a
libutils_a_SOURCES = Time.cpp Stats.cpp
libutils_a_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src

if USE_RDMA
libutils_a_SOURCES += InfinibandSupport.cpp
endif
