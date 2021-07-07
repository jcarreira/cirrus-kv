include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = 	store_latency 1_3 throughput iterator_benchmark \
                cache_benchmark 1_2 outstanding_requests store_bandwidth \
                cache_latency cache_iops iterator_v2 remove_bulk

LIBS         = 	-lclient -lauthentication -lutils -lcommon -levictionpolicies \
		$(LIBRDMACM) $(LIBIBVERBS)

LINCLUDES    = 	-L$(top_srcdir)/src/utils/ \
		-L$(top_srcdir)/src/authentication \
		-L$(top_srcdir)/src/common \
		-L$(top_srcdir)/src/cache_manager \
		-L$(top_srcdir)/src/client/

LDFLAGS  = -pthread
LDADD 	 = $(LINCLUDES) $(LIBS)

CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
           -isystem $(top_srcdir)/third_party/libcuckoo/ \
           -I$(top_srcdir)/src \
	   -I$(top_srcdir)/third_party/flatbuffers/include

store_latency_SOURCES  = store_latency.cpp

1_2_SOURCES  = 1_2.cpp

1_3_SOURCES  = 	1_3.cpp

throughput_SOURCES  = throughput.cpp

iterator_benchmark_SOURCES  = iterator_benchmark.cpp

iterator_v2_SOURCES = iterator_v2.cpp

cache_benchmark_SOURCES  = cache_benchmark.cpp

cache_latency_SOURCES = cache_latency.cpp

cache_iops_SOURCES = cache_iops.cpp

store_bandwidth_SOURCES = store_bandwidth.cpp

outstanding_requests_SOURCES  = outstanding_requests.cpp

remove_bulk_SOURCES  = remove_bulk.cpp
