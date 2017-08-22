AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = parameter_server two_clients_test bug_regression

TOP_DIR = $(top_srcdir)/../../

LIBS         =  -l:libcityhash.a -lclient \ 
                -lauthentication -lutils -lcommon
LINCLUDES    = -L$(TOP_DIR)/src/utils/ \
	       -L$(TOP_DIR)/src/client/ \
	       -L$(TOP_DIR)/src/ \
	       -L$(TOP_DIR)/src/authentication \
	       -L$(TOP_DIR)/src/common \
	       -L$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/

LDADD    = $(LINCLUDES) $(LIBS)
LDFLAGS  = -pthread
CPPFLAGS = -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
           -I$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
           -I$(TOP_DIR)/src \
           -I$(TOP_DIR)/third_party/flatbuffers/include \
           -I$(TOP_DIR)/third_party/eigen_source \
           -isystem $(TOP_DIR)/third_party/libcuckoo/

CPP_SOURCES = Input.cpp Utils.cpp \
              Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp \
              ModelGradient.cpp MlUtils.cpp Configuration.cpp \
              Checksum.cpp SoftmaxModel.cpp Serializers.cpp \
              Tasks.cpp

parameter_server_SOURCES  = parameter_server.cpp $(CPP_SOURCES)
two_clients_test_SOURCES  = two_clients_test.cpp
bug_regression_SOURCES  = bug_regression.cpp $(CPP_SOURCES)
