include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

DEFAULT_LIB = -L. -lserver \
              -L../authentication/ -lauthentication \
              -L../utils/ -lutils \
              -L../../third_party/rocksdb/ -lrocksdb -lsnappy -lbz2 -lz \
              -L../common/ -lcommon $(LIBRDMACM) $(LIBIBVERBS)

noinst_LIBRARIES = libserver.a
bin_PROGRAMS = tcpservermain

libserver_a_SOURCES = TCPServer.cpp MemoryBackend.cpp
libserver_a_CPPFLAGS = -ggdb -I$(top_srcdir) \
                       -I$(top_srcdir)/third_party/flatbuffers/include \
                       -isystem $(top_srcdir)/third_party/rocksdb/include \
                       -I$(top_srcdir)/src

if USE_RDMA
bin_PROGRAMS += bladepoolmain bladeallocmain allocmain controllermain \
                objectstoremain
libserver_a_SOURCES += RDMAServer.cpp BladePoolServer.cpp \
                      ConnectionContext.cpp \
                      ResourceAllocator.cpp BladeAllocServer.cpp \
                      BladeObjectStore.cpp MemoryBackend.cpp NVStorageBackend.cpp
endif

LDFLAGS  = -pthread
LDADD    = $(DEFAULT_LIB)
CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src

bladepoolmain_DEPENDENCIES = libserver.a
bladepoolmain_SOURCES  = bladepoolmain.cpp

bladeallocmain_DEPENDENCIES = libserver.a
bladeallocmain_SOURCES  = bladeallocmain.cpp

allocmain_DEPENDENCIES = libserver.a
allocmain_SOURCES  = allocmain.cpp

controllermain_DEPENDENCIES = libserver.a
controllermain_SOURCES  = controllermain.cpp

objectstoremain_DEPENDENCIES = libserver.a
objectstoremain_SOURCES  = objectstoremain.cpp

tcpservermain_DEPENDENCIES = libserver.a
tcpservermain_SOURCES  = TCPServerMain.cpp
