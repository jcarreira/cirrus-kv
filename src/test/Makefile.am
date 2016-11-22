AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = test_fullblade_store

test_fullblade_store_SOURCES  = test_fullblade_store.cpp
test_fullblade_store_LDFLAGS  = -pthread
test_fullblade_store_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
				-I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
test_fullblade_store_LDADD    = -L$(top_srcdir)/src/utils/ \
				-L$(top_srcdir)/src/client/ \
				-L$(top_srcdir)/src/authentication \
				-L$(top_srcdir)/src/common \
				-L$(top_srcdir)/src/object_store \
				-L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/ \
                                -lobjstore -lcityhash -lclient -libverbs -lrdmacm -lauthentication -lutils -lcommon
