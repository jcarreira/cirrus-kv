include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = tcpclientmain

LIBS          = -lclient -lutils -lauthentication -lcommon

LINCLUDES     = -L$(top_srcdir)/src/utils/ \
	        -L$(top_srcdir)/src/client/ \
    	        -L$(top_srcdir)/src/authentication \
	        -L$(top_srcdir)/src/common \
     	        -L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/ \
	        $(LIBRDMACM) $(LIBIBVERBS)

if USE_RDMA
bin_PROGRAMS += rdmaclientmain
endif

CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG) \
	   -I$(top_srcdir)/third_party/flatbuffers/include \
	   -I$(top_srcdir)/src \
	   -isystem $(top_srcdir)/third_party/libcuckoo/
LDFLAGS = -pthread

LDADD = $(LIBS) $(LINCLUDES)

rdmaclientmain_SOURCES  = RDMAClientmain.cpp

tcpclientmain_SOURCES  = TCPClientMain.cpp
