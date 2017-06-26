AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = tcpclientmain rdmaclientmain

LIBS    = -l:libcityhash.a -libverbs -lrdmacm -lclient -L../utils/ -lutils -L../authentication/ -lauthentication \
		  -L../common/ -lcommon -L. -L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/


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
