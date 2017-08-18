include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS =  pr pr2

LIBS         =  -lcityhash -lclient \
                -lauthentication -lutils -lcommon -levictionpolicies \
                $(LIBRDMACM) $(LIBIBVERBS)

LINCLUDES    =  -L$(top_srcdir)/src/utils/ \
                -L$(top_srcdir)/src/client/ \
                -L$(top_srcdir)/src/authentication \
                -L$(top_srcdir)/src/common \
                -L$(top_srcdir)/src/cache_manager \
                -L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

CPPFLAGS =  -ggdb -O3 -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
		-isystem $(top_srcdir)/third_party/libcuckoo/ \
		-I$(top_srcdir)/src \
		-I$(top_srcdir)/third_party/flatbuffers/include
SOURCES1  =  PageRank.cpp Input.cpp Vertex.cpp
SOURCES2  =  PageRank2.cpp Input.cpp Vertex.cpp

pr_SOURCES  =  pr.cpp $(SOURCES1)
pr_LDFLAGS  =  -pthread
pr_LDADD    =  $(LINCLUDES) $(LIBS)

pr2_SOURCES  =  pr2.cpp $(SOURCES2)
pr2_LDFLAGS  =  -pthread
pr2_LDADD    =  $(LINCLUDES) $(LIBS)

#-I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3

