AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = test_store_v2 exhaustion test_fullblade_store test_mt \
               test_mult_clients test_cache_manager iterator

LIBS         = -l:libcityhash.a -lclient -libverbs -lrdmacm\
               -lauthentication -lutils -lcommon
LINCLUDES    = -L$(top_srcdir)/src/utils/ \
		-L$(top_srcdir)/src/client/ \
		-L$(top_srcdir)/src/authentication \
		-L$(top_srcdir)/src/common \
		-L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

test_fullblade_store_SOURCES  = test_fullblade_store.cpp
test_fullblade_store_LDFLAGS  = -pthread
test_fullblade_store_CPPFLAGS = -ggdb -I$(top_srcdir)\
                                -I$(top_srcdir)/examples/sparsehash/src/ \
				-I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
test_fullblade_store_LDADD    = $(LINCLUDES) \
                                $(LIBS)

test_mt_SOURCES               = test_mt.cpp
test_mt_LDFLAGS               = -pthread
test_mt_CPPFLAGS              = -ggdb -I$(top_srcdir)\
                                -I$(top_srcdir)/examples/sparsehash/src/ \
				-I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/\
                                -O3
test_mt_LDADD                 = $(LINCLUDES) \
                                $(LIBS)

test_mult_clients_SOURCES     = test_mult_clients.cpp
test_mult_clients_LDFLAGS     = -pthread
test_mult_clients_CPPFLAGS    = -ggdb -I$(top_srcdir)\
                                -I$(top_srcdir)/examples/sparsehash/src/ \
                    		-I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
test_mult_clients_LDADD       = $(LINCLUDES) \
                                $(LIBS)

test_store_v2_SOURCES     = test_store_v2.cpp
test_store_v2_LDFLAGS     = -pthread
test_store_v2_CPPFLAGS    = -ggdb -I$(top_srcdir)\
                            -I$(top_srcdir)/examples/sparsehash/src/ \
                            -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
test_store_v2_LDADD = $(LINCLUDES) $(LIBS)

exhaustion_SOURCES  = mem_exhaustion.cpp
exhaustion_LDFLAGS  = -pthread
exhaustion_CPPFLAGS = -ggdb -I$(top_srcdir) \
                      -I$(top_srcdir)/examples/sparsehash/src/ \
                      -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
exhaustion_LDADD    = $(LINCLUDES) \
                      $(LIBS)

test_cache_manager_SOURCES  = test_cache_manager.cpp
test_cache_manager_LDFLAGS  = -pthread
test_cache_manager_CPPFLAGS = -ggdb -I$(top_srcdir) \
                      	            -I$(top_srcdir)/examples/sparsehash/src/ \
                                    -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
test_cache_manager_LDADD    = $(LINCLUDES) \
                               $(LIBS)

iterator_SOURCES  = test_iterator.cpp
iterator_LDFLAGS  = -pthread
iterator_CPPFLAGS = -ggdb -I$(top_srcdir) \
                     	        -I$(top_srcdir)/examples/sparsehash/src/ \
                              -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
iterator_LDADD    = $(LINCLUDES) $(LIBS)
