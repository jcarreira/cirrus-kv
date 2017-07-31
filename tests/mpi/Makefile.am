AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = test_mpi test_multi_tasks

TOP_DIR = $(top_srcdir)/../../

LIBS         =  -l:libcityhash.a -lclient \ 
                -lauthentication -lutils -lcommon
LINCLUDES    = -L$(TOP_DIR)/src/utils/ \
	       -L$(TOP_DIR)/src/client/ \
	       -L$(TOP_DIR)/src/authentication \
	       -L$(TOP_DIR)/src/common \
	       -L$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

test_mpi_SOURCES  = test_mpi.cpp
test_mpi_LDFLAGS  = -pthread
test_mpi_CPPFLAGS = -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
		    -I$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
                    -I$(TOP_DIR)/src \
		    -I$(TOP_DIR)/third_party/flatbuffers/include
test_mpi_LDADD    = $(LINCLUDES) $(LIBS)

    
    
test_multi_tasks_SOURCES  = test_multi_tasks.cpp
test_multi_tasks_LDFLAGS  = -pthread
test_multi_tasks_CPPFLAGS = -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
                	    -I$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
                            -I$(TOP_DIR)/src \
                	    -I$(TOP_DIR)/third_party/flatbuffers/include
test_multi_tasks_LDADD    = $(LINCLUDES) $(LIBS)
