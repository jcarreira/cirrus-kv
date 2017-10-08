TOP_DIR = $(top_srcdir)/../../

include $(TOP_DIR)/common.mk
AUTOMAKE_OPTIONS = foreign

CXX = g++
#CXX = $(MPI_HOME)/bin/mpic++

bin_PROGRAMS = parameter_server
#two_clients_test bug_regression \
#               parameter_server_softmax

LIBS         =  -lclient -lauthentication -lutils \
		-levictionpolicies

if USE_RDMA
LIBS += -lrdmacm -libverbs
endif

LINCLUDES    = -L$(TOP_DIR)/src/utils/ \
	       -L$(TOP_DIR)/src/client/ \
	       -L$(TOP_DIR)/src/ \
	       -L$(TOP_DIR)/src/authentication \
	       -L$(TOP_DIR)/src/common \
	       -L$(TOP_DIR)/src/cache_manager

LDADD    = $(LINCLUDES) $(LIBS)
LDFLAGS  = -static  -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
CPPFLAGS = -static -static-libgcc -static-libstdc++ \
           -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
           -I$(TOP_DIR)/src \
           -I$(TOP_DIR)/third_party/flatbuffers/include \
           -I$(TOP_DIR)/third_party/eigen_source \
           -isystem $(TOP_DIR)/third_party/libcuckoo/ \
           -I $(MPI_HOME)/include/

CPP_SOURCES = Input.cpp Utils.cpp \
              Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp \
              ModelGradient.cpp MlUtils.cpp Configuration.cpp \
              Checksum.cpp SoftmaxModel.cpp Serializers.cpp \
              Tasks.cpp

CPP_SOURCES_SOFTMAX = Input.cpp Utils.cpp \
              Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp \
              ModelGradient.cpp MlUtils.cpp Configuration.cpp \
              Checksum.cpp SoftmaxModel.cpp \
              Tasks_softmax.cpp Serializers.cpp 

parameter_server_SOURCES  = parameter_server.cpp $(CPP_SOURCES)
parameter_server_softmax_SOURCES  = parameter_server_softmax.cpp \
				$(CPP_SOURCES_SOFTMAX)
two_clients_test_SOURCES  = two_clients_test.cpp $(CPP_SOURCES)
bug_regression_SOURCES  = bug_regression.cpp $(CPP_SOURCES)
