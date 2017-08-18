include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS =  test_store_v2 exhaustion test_fullblade_store test_mt \
		test_mult_clients test_cache_manager test_iterator

LIBS         = -lclient -lauthentication -lutils -lcommon -levictionpolicies \
		$(LIBRDMACM) $(LIBIBVERBS)

LINCLUDES    =  -L$(top_srcdir)/src/utils/ \
		-L$(top_srcdir)/src/client/ \
		-L$(top_srcdir)/src/authentication \
		-L$(top_srcdir)/src/common \
		-L$(top_srcdir)/src/cache_manager

LDFLAGS  = -pthread
LDADD 	 = $(LINCLUDES) $(LIBS)
CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src \
	   -I$(top_srcdir)/examples/sparsehash/src/ \
	   -isystem $(top_srcdir)/third_party/libcuckoo/ \
	   -I$(top_srcdir)/third_party/flatbuffers/include

test_fullblade_store_SOURCES  = test_fullblade_store.cpp

test_mt_SOURCES               = test_mt.cpp

test_mult_clients_SOURCES     = test_mult_clients.cpp

test_store_v2_SOURCES         = test_store_v2.cpp

exhaustion_SOURCES	      = mem_exhaustion.cpp

test_cache_manager_SOURCES    = test_cache_manager.cpp

test_iterator_SOURCES         = test_iterator.cpp
