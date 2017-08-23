include $(top_srcdir)/common.mk
AUTOMAKE_OPTIONS = foreign

CXX = mpic++
#CXX = $(MPI_HOME)/bin/mpic++

bin_PROGRAMS = parameter_server two_clients_test bug_regression

#TOP_DIR = $(top_srcdir)/../../
 
LIBS         =  -lclient -lauthentication -lutils -lmpi -lrdmacm -libverbs

LINCLUDES    = -L$(top_srcdir)/src/utils/ \
	       -L$(top_srcdir)/src/client/ \
	       -L$(top_srcdir)/src/ \
	       -L$(top_srcdir)/src/authentication \
	       -L$(top_srcdir)/src/common

LDADD    = $(LINCLUDES) $(LIBS)
LDFLAGS  = -pthread
CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/examples/sparsehash/src/ \
           -I$(top_srcdir)/src \
           -I$(top_srcdir)/third_party/flatbuffers/include \
           -I$(top_srcdir)/third_party/eigen_source \
           -isystem $(top_srcdir)/third_party/libcuckoo/ \
           -I $(MPI_HOME)/include/

CPP_SOURCES = Input.cpp Utils.cpp \
              Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp \
              ModelGradient.cpp MlUtils.cpp Configuration.cpp \
              Checksum.cpp SoftmaxModel.cpp Serializers.cpp \
              Tasks.cpp

parameter_server_SOURCES  = parameter_server.cpp $(CPP_SOURCES)
two_clients_test_SOURCES  = two_clients_test.cpp $(CPP_SOURCES)
bug_regression_SOURCES  = bug_regression.cpp $(CPP_SOURCES)
