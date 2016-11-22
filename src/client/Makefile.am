include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

#DEFINE_LOG=-DELPP_DISABLE_INFO_LOGS

SOURCES = RDMAClient.cpp BladeClient.cpp AuthenticationClient.cpp BladeFileClient.cpp
LIBS    = -lclient -libverbs -lrdmacm -L../utils/ -lutils -L../authentication/ -lauthentication \
          -L../common/ -lcommon -L.

noinst_LIBRARIES = libclient.a
bin_PROGRAMS = clientmain scalability_main fileclientmain

libclient_a_SOURCES = $(SOURCES)
libclient_a_CPPFLAGS = -ggdb -I$(top_srcdir)

clientmain_SOURCES  = clientmain.cpp 
clientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG)
clientmain_LDFLAGS  = -pthread
clientmain_LDADD    = $(LIBS)

fileclientmain_SOURCES  = fileclientmain.cpp
fileclientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG)
fileclientmain_LDFLAGS  = -pthread
fileclientmain_LDADD    = $(LIBS)

scalability_main_SOURCES  = scalability_main.cpp
scalability_main_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG)
scalability_main_LDFLAGS  = -pthread
scalability_main_LDADD     = $(LIBS)

