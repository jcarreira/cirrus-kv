include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = 	1_1 1_3 throughput iterator_benchmark \
                cache_benchmark 1_2 outstanding_requests

LIBS         = 	-lclient \
               	-lauthentication -lutils -lcommon -levictionpolicies \
		$(LIBRDMACM) $(LIBIBVERBS)

LINCLUDES    = 	-L$(top_srcdir)/src/utils/ \
		-L$(top_srcdir)/src/authentication \
		-L$(top_srcdir)/src/common \
		-L$(top_srcdir)/src/cache_manager \
		-L$(top_srcdir)/src/client/

1_1_SOURCES  = 	1_1.cpp
1_1_LDFLAGS  = 	-pthread
1_1_CPPFLAGS = 	-ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
               	-I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
               	-I$(top_srcdir)/src \
		-I$(top_srcdir)/third_party/flatbuffers/include
1_1_LDADD    = 	$(LINCLUDES) $(LIBS)

1_2_SOURCES  = 1_2.cpp
1_2_LDFLAGS  = -pthread
1_2_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
               -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
               -I$(top_srcdir)/src \
	       -I$(top_srcdir)/third_party/flatbuffers/include
1_2_LDADD    = $(LINCLUDES) $(LIBS)

1_3_SOURCES  = 	1_3.cpp
1_3_LDFLAGS  = 	-pthread
1_3_CPPFLAGS = 	-ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
               	-I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
               	-I$(top_srcdir)/src \
		-I$(top_srcdir)/third_party/flatbuffers/include
1_3_LDADD    = 	$(LINCLUDES) $(LIBS)

throughput_SOURCES  = throughput.cpp
throughput_LDFLAGS  = -pthread
throughput_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
                      -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
                      -I$(top_srcdir)/src \
                      -I$(top_srcdir)/third_party/flatbuffers/include
throughput_LDADD    = $(LINCLUDES) $(LIBS)

iterator_benchmark_SOURCES  = iterator_benchmark.cpp
iterator_benchmark_LDFLAGS  = -pthread
iterator_benchmark_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src \
			      -I$(top_srcdir)/examples/sparsehash/src/ \
			      -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 -I$(top_srcdir)/third_party/flatbuffers/include
iterator_benchmark_LDADD    = $(LINCLUDES) $(LIBS)

cache_benchmark_SOURCES  = cache_benchmark.cpp
cache_benchmark_LDFLAGS  = -pthread
cache_benchmark_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src \
			   -I$(top_srcdir)/examples/sparsehash/src/ \
			    -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 -I$(top_srcdir)/third_party/flatbuffers/include
cache_benchmark_LDADD    = $(LINCLUDES) $(LIBS)

outstanding_requests_SOURCES  = outstanding_requests.cpp
outstanding_requests_LDFLAGS  = -pthread
outstanding_requests_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src \
			      -I$(top_srcdir)/examples/sparsehash/src/ \
			      -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 -I$(top_srcdir)/third_party/flatbuffers/include
outstanding_requests_LDADD    = $(LINCLUDES) $(LIBS)
