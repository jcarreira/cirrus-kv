include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

LIBS         = -lobjstore
LINCLUDES    = -L$(top_srcdir)/src/object_store

noinst_LIBRARIES = libiterator.a
libiterator_a_SOURCES = Iterator.cpp
libiterator_a_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/
