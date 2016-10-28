include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

DEFAULT_LIB=-libverbs -lrdmacm -L../utils/ -lutils \
            -L../common/ -lcommon -L../authentication/ -lauthentication

bin_PROGRAMS = bladepoolmain bladeallocmain allocmain controllermain
noinst_LIBRARIES = libserver.a

bladepoolmain_SOURCES  = bladepoolmain.cpp RDMAServer.cpp BladePoolServer.cpp \
                    ConnectionContext.cpp DumbQuotaManager.cpp BladeAllocServer.cpp
bladepoolmain_LDFLAGS  = -pthread
bladepoolmain_CPPFLAGS = -ggdb -I$(top_srcdir)
bladepoolmain_LDADD    = $(DEFAULT_LIB)

bladeallocmain_SOURCES  = bladeallocmain.cpp RDMAServer.cpp BladePoolServer.cpp \
                    ConnectionContext.cpp DumbQuotaManager.cpp BladeAllocServer.cpp
bladeallocmain_LDFLAGS  = -pthread
bladeallocmain_CPPFLAGS = -ggdb -I$(top_srcdir)
bladeallocmain_LDADD    = $(DEFAULT_LIB)


allocmain_SOURCES  = allocmain.cpp RDMAServer.cpp ResourceAllocator.cpp \
                     ConnectionContext.cpp DumbQuotaManager.cpp BladeAllocServer.cpp
allocmain_LDFLAGS  = -pthread
allocmain_CPPFLAGS = -ggdb -I$(top_srcdir)
allocmain_LDADD    = $(DEFAULT_LIB)


controllermain_SOURCES  = controllermain.cpp RDMAServer.cpp ResourceAllocator.cpp \
                     ConnectionContext.cpp DumbQuotaManager.cpp BladeAllocServer.cpp
controllermain_LDFLAGS  = -pthread
controllermain_CPPFLAGS = -ggdb -I$(top_srcdir)
controllermain_LDADD    = $(DEFAULT_LIB)


libserver_a_SOURCES = RDMAServer.cpp BladePoolServer.cpp \
                     ConnectionContext.cpp DumbQuotaManager.cpp \
                     ResourceAllocator.cpp BladeAllocServer.cpp
libserver_a_CPPFLAGS = -ggdb -I$(top_srcdir)
