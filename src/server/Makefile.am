include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

DEFAULT_LIB = -L. -lserver \
              -L../authentication/ -lauthentication \
              -L../utils/ -lutils \
              -L../common/ -lcommon $(LIBRDMACM) $(LIBIBVERBS)

noinst_LIBRARIES = libserver.a
bin_PROGRAMS = tcpservermain testtcpserver testtcpclient

libserver_a_SOURCES = TCPServer.cpp
libserver_a_CPPFLAGS = -ggdb -I$(top_srcdir) \
                       -I$(top_srcdir)/third_party/flatbuffers/include \
                       -I$(top_srcdir)/src

if USE_RDMA
bin_PROGRAMS += bladepoolmain bladeallocmain allocmain controllermain \
                objectstoremain
libserver_a_SOURCES += RDMAServer.cpp BladePoolServer.cpp \
                       ConnectionContext.cpp \
                       ResourceAllocator.cpp BladeAllocServer.cpp \
                       BladeObjectStore.cpp
endif

LDFLAGS  = -pthread
LDADD    = $(DEFAULT_LIB)
CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src

testtcpserver_DEPENDENCIES = libserver.a
testtcpserver_SOURCES = TestTCPServer.cpp
testtcpclient_DEPENDENCIES = libserver.a
testtcpclient_SOURCES = TestTCPClient.cpp

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
