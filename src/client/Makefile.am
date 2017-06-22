include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

#DEFINE_LOG=-DELPP_DISABLE_INFO_LOGS

SOURCES = RDMAClient.cpp TCPClient.cpp
LIBS    = -lclient -libverbs -lrdmacm -L../utils/ -lutils -L../authentication/ -lauthentication \
					-L../common/ -lcommon -L.

noinst_LIBRARIES = libclient.a
bin_PROGRAMS = clientmain scalability_main tcpclientmain

libclient_a_SOURCES = $(SOURCES)
libclient_a_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src \
			-I$(top_srcdir)/third_party/flatbuffers/include

clientmain_DEPENDENCIES = libclient.a
clientmain_SOURCES  = clientmain.cpp
clientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG) -I$(top_srcdir)/src
clientmain_LDFLAGS  = -pthread
clientmain_LDADD    = $(LIBS)

scalability_main_DEPENDENCIES = libclient.a
scalability_main_SOURCES  = scalability_main.cpp
scalability_main_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG) -I$(top_srcdir)/src
scalability_main_LDFLAGS  = -pthread
scalability_main_LDADD     = $(LIBS)

tcpclientmain_DEPENDENCIES = libclient.a
tcpclientmain_SOURCES  = TCPClientMain.cpp
tcpclientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG) \
						 -I$(top_srcdir)/third_party/flatbuffers/include \
						 -I$(top_srcdir)/src
tcpclientmain_LDFLAGS  = -pthread
tcpclientmain_LDADD    = $(LIBS)
