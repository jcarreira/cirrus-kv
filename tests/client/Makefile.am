AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = tcpclientmain

LIBS          = -l:libcityhash.a -lclient -L../utils/ -lutils -L../authentication/ -lauthentication \
                -L../common/ -lcommon -L. -L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

LINCLUDES    = -L$(top_srcdir)/src/utils/ \
	       -L$(top_srcdir)/src/client/ \
    	       -L$(top_srcdir)/src/authentication \
	       -L$(top_srcdir)/src/common \
     	       -L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/ \
	       $(LIBRDMACM) $(LIBIBVERBS)

if USE_RDMA
bin_PROGRAMS += rdmaclientmain
endif

rdmaclientmain_SOURCES  = RDMAClientmain.cpp
rdmaclientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG) \
			  -I$(top_srcdir)/third_party/flatbuffers/include \
			  -I$(top_srcdir)/src \
			  -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src
rdmaclientmain_LDFLAGS  = -pthread
rdmaclientmain_LDADD    = $(LIBS) $(LINCLUDES)

tcpclientmain_SOURCES  = TCPClientMain.cpp
tcpclientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG) \
		         -I$(top_srcdir)/third_party/flatbuffers/include \
		         -I$(top_srcdir)/src \
			 -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src
tcpclientmain_LDFLAGS  = -pthread
tcpclientmain_LDADD    = $(LIBS) $(LINCLUDES)
