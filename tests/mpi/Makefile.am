AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = test_mpi

TOP_DIR = $(top_srcdir)/../../

LIBS         = -lobjstore -l:libcityhash.a -lclient -libverbs -lrdmacm\
               -lauthentication -lutils -lcommon
LINCLUDES    = -L$(TOP_DIR)/src/utils/ \
		-L$(TOP_DIR)/src/client/ \
		-L$(TOP_DIR)/src/authentication \
		-L$(TOP_DIR)/src/common \
		-L$(TOP_DIR)/src/object_store \
		-L$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

test_mpi_SOURCES  = test_mpi.cpp
test_mpi_LDFLAGS  = -pthread
#--static -L$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/
test_mpi_CPPFLAGS = -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
			-I$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3
test_mpi_LDADD    = $(LINCLUDES) $(LIBS)

