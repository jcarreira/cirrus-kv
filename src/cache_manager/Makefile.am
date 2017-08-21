include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libevictionpolicies.a
libevictionpolicies_a_SOURCES = LRAddedEvictionPolicy.cpp LRUEvictionPolicy.cpp

libevictionpolicies_a_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src
