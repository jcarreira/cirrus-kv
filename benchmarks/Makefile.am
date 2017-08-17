include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = 	1_1 1_3 throughput iterator_benchmark \
                cache_benchmark 1_2 outstanding_requests

LIBS         = 	-lclient -lauthentication -lutils -lcommon -levictionpolicies \
		$(LIBRDMACM) $(LIBIBVERBS)

LINCLUDES    = 	-L$(top_srcdir)/src/utils/ \
		-L$(top_srcdir)/src/client/ \
		-L$(top_srcdir)/src/authentication \
		-L$(top_srcdir)/src/common \
		-L$(top_srcdir)/src/cache_manager

LDFLAGS  = -pthread
LDADD 	 = $(LINCLUDES) $(LIBS)

CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
           -isystem $(top_srcdir)/third_party/libcuckoo/ \
           -I$(top_srcdir)/src \
	   -I$(top_srcdir)/third_party/flatbuffers/include

1_1_SOURCES  = 	1_1.cpp

1_2_SOURCES  = 1_2.cpp

1_3_SOURCES  = 	1_3.cpp

throughput_SOURCES  = throughput.cpp

iterator_benchmark_SOURCES  = iterator_benchmark.cpp

cache_benchmark_SOURCES  = cache_benchmark.cpp

outstanding_requests_SOURCES  = outstanding_requests.cpp
