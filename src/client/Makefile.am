include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

#DEFINE_LOG=-DELPP_DISABLE_INFO_LOGS

SOURCES = RDMAClient.cpp BladeClient.cpp AuthenticationClient.cpp BladeFileClient.cpp
LIBS    = -libverbs -lrdmacm -L../utils/ -lutils -L../authentication/ -lauthentication \
          -L../common/ -lcommon

bin_PROGRAMS = clientmain scalability_main fileclientmain
noinst_LIBRARIES = libclient.a

clientmain_SOURCES  = clientmain.cpp $(SOURCES)
clientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG)
clientmain_LDFLAGS  = -pthread
clientmain_LDADD    = $(LIBS)

fileclientmain_SOURCES  = fileclientmain.cpp $(SOURCES)
fileclientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG)
fileclientmain_LDFLAGS  = -pthread
fileclientmain_LDADD    = $(LIBS)

scalability_main_SOURCES  = scalability_main.cpp $(SOURCES)
scalability_main_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG)
scalability_main_LDFLAGS  = -pthread
scalability_main_LDADD     = $(LIBS)

libclient_a_SOURCES = $(SOURCES)
libclient_a_CPPFLAGS = -ggdb -I$(top_srcdir)
