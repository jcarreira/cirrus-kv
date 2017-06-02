AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = test_atomics
LIBS         = -lobjstore -lcityhash -lclient -libverbs -lrdmacm\
               -lauthentication -lutils -lcommon
LINCLUDES    = -L$(top_srcdir)/src/utils/ \
		-L$(top_srcdir)/src/client/ \
		-L$(top_srcdir)/src/authentication \
		-L$(top_srcdir)/src/common \
		-L$(top_srcdir)/src/object_store \
		-L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

test_atomics_SOURCES  = test_atomics.cpp
test_atomics_LDFLAGS  = -pthread
test_atomics_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
                        -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
test_atomics_LDADD    = $(LINCLUDES) \
                        $(LIBS)

