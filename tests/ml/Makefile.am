AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = parameter_server

TOP_DIR = $(top_srcdir)/../../

LIBS         =  -l:libcityhash.a -lclient \ 
                -lauthentication -lutils -lcommon
LINCLUDES    = -L$(TOP_DIR)/src/utils/ \
	       -L$(TOP_DIR)/src/client/ \
	       -L$(TOP_DIR)/src/ \
	       -L$(TOP_DIR)/src/authentication \
	       -L$(TOP_DIR)/src/common \
	       -L$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

parameter_server_SOURCES  = parameter_server.cpp Input.cpp Utils.cpp Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp ModelGradient.cpp MlUtils.cpp
parameter_server_LDFLAGS  = -pthread
parameter_server_CPPFLAGS = -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
                	    -I$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
                            -I$(TOP_DIR)/src \
                	    -I$(TOP_DIR)/third_party/flatbuffers/include \
                	    -I$(TOP_DIR)/third_party/eigen_source
parameter_server_LDADD    = $(LINCLUDES) $(LIBS)
