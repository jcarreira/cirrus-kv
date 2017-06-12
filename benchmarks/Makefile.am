AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = 1_1 1_2 1_3 throughput
LIBS         = -l:libcityhash.a -lclient -libverbs -lrdmacm\
               -lauthentication -lutils -lcommon
LINCLUDES    = -L$(top_srcdir)/src/utils/ \
		-L$(top_srcdir)/src/client/ \
		-L$(top_srcdir)/src/authentication \
		-L$(top_srcdir)/src/common \
		-L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

1_1_SOURCES  = 1_1.cpp
1_1_LDFLAGS  = -pthread
1_1_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
               -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
1_1_LDADD    = $(LINCLUDES) \
               $(LIBS)

1_2_SOURCES  = 1_2.cpp
1_2_LDFLAGS  = -pthread
1_2_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
               -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
1_2_LDADD    = $(LINCLUDES) \
               $(LIBS)

1_3_SOURCES  = 1_3.cpp
1_3_LDFLAGS  = -pthread
1_3_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
               -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
1_3_LDADD    = $(LINCLUDES) \
               $(LIBS)

 throughput_SOURCES  = throughput.cpp
 throughput_LDFLAGS  = -pthread
 throughput_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
                       -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
 throughput_LDADD    = $(LINCLUDES) \
                        $(LIBS)
