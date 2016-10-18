include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = blademain allocmain
noinst_LIBRARIES = libserver.a

blademain_SOURCES  = blademain.cpp RDMAServer.cpp BladeServer.cpp \
                    ConnectionContext.cpp DumbQuotaManager.cpp
blademain_LDFLAGS  = -pthread
blademain_CPPFLAGS = -ggdb -I$(top_srcdir)
blademain_LDADD    = -libverbs -lrdmacm -L../utils/ -lutils


allocmain_SOURCES  = allocmain.cpp RDMAServer.cpp ResourceAllocator.cpp \
                     ConnectionContext.cpp DumbQuotaManager.cpp
allocmain_LDFLAGS  = -pthread
allocmain_CPPFLAGS = -ggdb -I$(top_srcdir)
allocmain_LDADD    = -libverbs -lrdmacm -L../utils/ -lutils


libserver_a_SOURCES = RDMAServer.cpp BladeServer.cpp \
                     ConnectionContext.cpp DumbQuotaManager.cpp \
                     ResourceAllocator.cpp
libserver_a_CPPFLAGS = -ggdb -I$(top_srcdir)
