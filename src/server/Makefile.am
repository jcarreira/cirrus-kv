include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

DEFAULT_LIB= -L. -lserver -lrdmacm -libverbs\
             -L../authentication/ -lauthentication \
            -L../utils/ -lutils \
            -L../common/ -lcommon

noinst_LIBRARIES = libserver.a
bin_PROGRAMS = bladepoolmain bladeallocmain allocmain controllermain bladefilemain

libserver_a_SOURCES = RDMAServer.cpp BladePoolServer.cpp \
                     ConnectionContext.cpp DumbQuotaManager.cpp \
                     ResourceAllocator.cpp BladeAllocServer.cpp \
                     BladeFileAllocServer.cpp
libserver_a_CPPFLAGS = -ggdb -I$(top_srcdir)


bladepoolmain_DEPENDENCIES = libserver.a
bladepoolmain_SOURCES  = bladepoolmain.cpp
bladepoolmain_LDFLAGS  = -pthread
bladepoolmain_CPPFLAGS = -ggdb -I$(top_srcdir)
bladepoolmain_LDADD    = $(DEFAULT_LIB)

bladeallocmain_DEPENDENCIES = libserver.a
bladeallocmain_SOURCES  = bladeallocmain.cpp
bladeallocmain_LDFLAGS  = -pthread
bladeallocmain_CPPFLAGS = -ggdb -I$(top_srcdir)
bladeallocmain_LDADD    = $(DEFAULT_LIB)

bladefilemain_DEPENDENCIES = libserver.a
bladefilemain_SOURCES  = bladefilemain.cpp
bladefilemain_LDFLAGS  = -pthread
bladefilemain_CPPFLAGS = -ggdb -I$(top_srcdir)
bladefilemain_LDADD    = $(DEFAULT_LIB)

allocmain_DEPENDENCIES = libserver.a
allocmain_SOURCES  = allocmain.cpp
allocmain_LDFLAGS  = -pthread
allocmain_CPPFLAGS = -ggdb -I$(top_srcdir)
allocmain_LDADD    = $(DEFAULT_LIB)

controllermain_DEPENDENCIES = libserver.a
controllermain_SOURCES  = controllermain.cpp
controllermain_LDFLAGS  = -pthread
controllermain_CPPFLAGS = -ggdb -I$(top_srcdir)
controllermain_LDADD    = $(DEFAULT_LIB)



