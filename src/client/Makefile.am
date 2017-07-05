include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

#DEFINE_LOG=-DELPP_DISABLE_INFO_LOGS

SOURCES = TCPClient.cpp BladeClient.cpp
LIBS    = -l:libcityhash.a -lclient -L../utils/ -lutils -L../authentication/ -lauthentication \
	  -L../common/ -lcommon -L. -L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

if USE_RDMA
SOURCES += RDMAClient.cpp
endif

noinst_LIBRARIES = libclient.a

libclient_a_SOURCES = $(SOURCES)
libclient_a_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src \
		       -I$(top_srcdir)/third_party/flatbuffers/include \
		       -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src
