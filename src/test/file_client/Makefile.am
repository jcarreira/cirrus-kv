AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = test_file
LIBS         = -lobjstore -lcityhash -lclient -libverbs -lrdmacm\
               -lauthentication -lutils -lcommon
LINCLUDES    = -L$(top_srcdir)/src/utils/ \
		-L$(top_srcdir)/src/client/ \
		-L$(top_srcdir)/src/authentication \
		-L$(top_srcdir)/src/common \
		-L$(top_srcdir)/src/object_store \
		-L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

test_file_SOURCES  = test_file.cpp
test_file_LDFLAGS  = -pthread
test_file_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
                     -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
test_file_LDADD    = $(LINCLUDES) \
                        $(LIBS)

