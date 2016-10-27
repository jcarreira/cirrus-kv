include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

DEFAULT_LIB=-libverbs -lrdmacm -L../utils/ -lutils \
            -L../common/ -lcommon -L../authentication/ -lauthentication

bin_PROGRAMS = blademain allocmain controllermain
noinst_LIBRARIES = libserver.a

blademain_SOURCES  = blademain.cpp RDMAServer.cpp BladePoolServer.cpp \
                    ConnectionContext.cpp DumbQuotaManager.cpp
blademain_LDFLAGS  = -pthread
blademain_CPPFLAGS = -ggdb -I$(top_srcdir)
blademain_LDADD    = $(DEFAULT_LIB)


allocmain_SOURCES  = allocmain.cpp RDMAServer.cpp ResourceAllocator.cpp \
                     ConnectionContext.cpp DumbQuotaManager.cpp
allocmain_LDFLAGS  = -pthread
allocmain_CPPFLAGS = -ggdb -I$(top_srcdir)
allocmain_LDADD    = $(DEFAULT_LIB)


controllermain_SOURCES  = controllermain.cpp RDMAServer.cpp ResourceAllocator.cpp \
                     ConnectionContext.cpp DumbQuotaManager.cpp
controllermain_LDFLAGS  = -pthread
controllermain_CPPFLAGS = -ggdb -I$(top_srcdir)
controllermain_LDADD    = $(DEFAULT_LIB)


libserver_a_SOURCES = RDMAServer.cpp BladePoolServer.cpp \
                     ConnectionContext.cpp DumbQuotaManager.cpp \
                     ResourceAllocator.cpp
libserver_a_CPPFLAGS = -ggdb -I$(top_srcdir)
