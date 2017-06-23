include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

#DEFINE_LOG=-DELPP_DISABLE_INFO_LOGS

SOURCES = RDMAClient.cpp TCPClient.cpp
LIBS    = -l:libcityhash.a -libverbs -lrdmacm -lclient -L../utils/ -lutils -L../authentication/ -lauthentication \
					-L../common/ -lcommon -L. -L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

noinst_LIBRARIES = libclient.a
bin_PROGRAMS = tcpclientmain rdmaclientmain

libclient_a_SOURCES = $(SOURCES)
libclient_a_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src \
					   -I$(top_srcdir)/third_party/flatbuffers/include \
					   -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src


rdmaclientmain_DEPENDENCIES = libclient.a
rdmaclientmain_SOURCES  = RDMAClientmain.cpp
rdmaclientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG) \
						  -I$(top_srcdir)/third_party/flatbuffers/include \
						  -I$(top_srcdir)/src \
						  -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src
rdmaclientmain_LDFLAGS  = -pthread
rdmaclientmain_LDADD    = $(LIBS)

tcpclientmain_DEPENDENCIES = libclient.a
tcpclientmain_SOURCES  = TCPClientMain.cpp
tcpclientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG) \
						 -I$(top_srcdir)/third_party/flatbuffers/include \
						 -I$(top_srcdir)/src \
						 -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src
tcpclientmain_LDFLAGS  = -pthread
tcpclientmain_LDADD    = $(LIBS)
