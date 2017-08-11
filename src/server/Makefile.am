include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

DEFAULT_LIB = -L. -lserver \
             -L../authentication/ -lauthentication \
             -L../utils/ -lutils \
             -L../common/ -lcommon $(LIBRDMACM) $(LIBIBVERBS)

noinst_LIBRARIES = libserver.a
bin_PROGRAMS = tcpservermain

libserver_a_SOURCES = TCPServer.cpp
libserver_a_CPPFLAGS = -ggdb -isystem $(top_srcdir)/third_party/libcuckoo/ \
                       -I$(top_srcdir) -I$(top_srcdir)/third_party/flatbuffers/include \
                       -I$(top_srcdir)/src

if USE_RDMA
bin_PROGRAMS += bladepoolmain bladeallocmain allocmain controllermain objectstoremain
libserver_a_SOURCES += RDMAServer.cpp BladePoolServer.cpp \
                      ConnectionContext.cpp \
                      ResourceAllocator.cpp BladeAllocServer.cpp \
                      BladeObjectStore.cpp
endif

bladepoolmain_DEPENDENCIES = libserver.a
bladepoolmain_SOURCES  = bladepoolmain.cpp
bladepoolmain_LDFLAGS  = -pthread
bladepoolmain_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src
bladepoolmain_LDADD    = $(DEFAULT_LIB)

bladeallocmain_DEPENDENCIES = libserver.a
bladeallocmain_SOURCES  = bladeallocmain.cpp
bladeallocmain_LDFLAGS  = -pthread
bladeallocmain_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src
bladeallocmain_LDADD    = $(DEFAULT_LIB)

allocmain_DEPENDENCIES = libserver.a
allocmain_SOURCES  = allocmain.cpp
allocmain_LDFLAGS  = -pthread
allocmain_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src
allocmain_LDADD    = $(DEFAULT_LIB)

controllermain_DEPENDENCIES = libserver.a
controllermain_SOURCES  = controllermain.cpp
controllermain_LDFLAGS  = -pthread
controllermain_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src
controllermain_LDADD    = $(DEFAULT_LIB)

objectstoremain_DEPENDENCIES = libserver.a
objectstoremain_SOURCES  = objectstoremain.cpp
objectstoremain_LDFLAGS  = -pthread
objectstoremain_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src
objectstoremain_LDADD    = $(DEFAULT_LIB)

tcpservermain_DEPENDENCIES = libserver.a
tcpservermain_SOURCES  = TCPServerMain.cpp
tcpservermain_LDFLAGS  = -pthread
tcpservermain_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src \
                         -isystem $(top_srcdir)/third_party/libcuckoo/
tcpservermain_LDADD    = $(DEFAULT_LIB)
