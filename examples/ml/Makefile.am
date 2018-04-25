TOP_DIR = $(top_srcdir)/../../

include $(TOP_DIR)/common.mk
AUTOMAKE_OPTIONS = foreign

CXX = g++
#CXX = $(MPI_HOME)/bin/mpic++

bin_PROGRAMS = parameter_server ps_test csv_to_libsvm

LIBS         =  -lclient -lauthentication -lutils \
		-levictionpolicies -lhiredis \
		-laws-cpp-sdk-s3 -laws-cpp-sdk-core \
		-ldl -lcurl -lkrb5 -lk5crypto -lall -lssl -lcrypto -lz \
		-lkeyutils -lhiredis -levent -lgflags

if USE_RDMA
LIBS += -lrdmacm -libverbs
endif

LINCLUDES    = -L$(TOP_DIR)/src/utils/ \
	       -L$(TOP_DIR)/src/client/ \
	       -L$(TOP_DIR)/src/ \
	       -L$(TOP_DIR)/src/authentication \
	       -L$(TOP_DIR)/src/common \
	       -L$(TOP_DIR)/src/cache_manager \
	       -L$(TOP_DIR)/third_party/aws-sdk-cpp/aws-cpp-sdk-core \
	       -L$(TOP_DIR)/third_party/aws-sdk-cpp/aws-cpp-sdk-s3 \
	       -L$(TOP_DIR)/third_party/curl/lib \
	       -L$(TOP_DIR)/third_party/gflags/lib \
	       -L$(TOP_DIR)/third_party/libevent/lib \
	       -L/home/ec2-user/kerberos/krb5-1.15.2/src/lib \
	       -L/home/ec2-user/keyutils/keyutils-1.5.10

LDADD    = $(LINCLUDES) $(LIBS)
LDFLAGS  = -static-libgcc -static -Wl,--whole-archive \
	   -pthread -Wl,--no-whole-archive
CPPFLAGS = -static-libgcc -static -static-libstdc++ \
           -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
           -I$(TOP_DIR)/src \
           -I$(TOP_DIR)/third_party/flatbuffers/include \
           -I$(TOP_DIR)/third_party/eigen_source \
           -I$(TOP_DIR)/third_party/hiredis \
           -isystem $(TOP_DIR)/third_party/libcuckoo/ \
           -I $(MPI_HOME)/include/ -Itest_aws \
	   -I$(TOP_DIR)/third_party/aws-sdk-cpp/aws-cpp-sdk-s3/include/ \
	   -I$(TOP_DIR)/third_party/aws-sdk-cpp/aws-cpp-sdk-core/include/  \
           -I$(TOP_DIR)/third_party/gflags/include/ \
	   -I/home/ec2-user/kerberos/krb5-1.15.2/src/include/ \
	   -ggdb -frecord-gcc-switches -O3
#-pg

CPP_SOURCES = InputReader.cpp Utils.cpp \
              Dataset.cpp Matrix.cpp \
	      Model.cpp LRModel.cpp SoftmaxModel.cpp SparseLRModel.cpp \
	      SparseMFModel.cpp MFModel.cpp\
              ModelGradient.cpp MlUtils.cpp Configuration.cpp \
              Checksum.cpp Serializers.cpp \
              ErrorSparseTask.cpp \
	      LogisticSparseTaskS3.cpp \
	      MFNetflixTask.cpp \
	      LoadingSparseTaskS3.cpp LoadingNetflixTask.cpp \
              TasksWait.cpp MurmurHash3.cpp \
	      RedisIterator.cpp  S3Iterator.cpp S3SparseIterator.cpp \
	      PerformanceLambdaTask.cpp \
	      Redis.cpp S3.cpp SparseDataset.cpp \
	      ProgressMonitor.cpp \
	      PSSparseServerTask.cpp PSSparseServerInterface.cpp

CPP_SOURCES_SOFTMAX = InputReader.cpp Utils.cpp \
              Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp \
              ModelGradient.cpp MlUtils.cpp Configuration.cpp \
              Checksum.cpp SoftmaxModel.cpp \
              Serializers.cpp  \
              Redis.cpp SparseDataset.cpp MurmurHash3.cpp

parameter_server_SOURCES  = parameter_server.cpp $(CPP_SOURCES)

ps_test_SOURCES = PSSparseServerTaskTest.cpp $(CPP_SOURCES)

csv_to_libsvm_SOURCES = CriteoCsvToLibSVM.cpp $(CPP_SOURCES)
