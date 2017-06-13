include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libcachemanager.a
libcachemanager_a_SOURCES = CacheManager.cpp
libcachemanager_a_CPPFLAGS = -ggdb -I$(top_srcdir) \
                             -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/
