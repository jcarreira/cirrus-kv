include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libobjstore.a
libobjstore_a_SOURCES = RDMAObjectStore.cpp ObjectStore.cpp EvictionPolicy.cpp RandomEvictionPolicy.cpp LRUEvictionPolicy.cpp
libobjstore_a_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ -I$(top_srcdir)/third_party/libcuckoo/ -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/
