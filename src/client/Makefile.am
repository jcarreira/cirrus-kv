include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

SOURCES = TCPClient.cpp BladeClient.cpp

LIBS    =  -lclient -L../utils/ -lutils -L../authentication/ -lauthentication \
	   -L../common/ -lcommon -L. $(LIBRDMACM) $(LIBIBVERBS)

if USE_RDMA
SOURCES += RDMAClient.cpp
endif

noinst_LIBRARIES = libclient.a

libclient_a_SOURCES = $(SOURCES)
libclient_a_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src \
		       -I$(top_srcdir)/third_party/flatbuffers/include \
			-isystem $(top_srcdir)/third_party/libcuckoo
