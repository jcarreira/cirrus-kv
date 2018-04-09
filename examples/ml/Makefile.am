TOP_DIR = $(top_srcdir)/../../

include $(TOP_DIR)/common.mk
AUTOMAKE_OPTIONS = foreign

CXX = g++
#CXX = $(MPI_HOME)/bin/mpic++

bin_PROGRAMS = parameter_server ps_test csv_to_libsvm

LIBS         =  -lclient -lauthentication -lutils \
		-levictionpolicies -lhiredis \
		-laws-cpp-sdk-s3 -laws-cpp-sdk-core \
		-lcurl -lssl -lcrypto -lz -ldl -lkrb5 -lk5crypto \
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
	       -L/home/ec2-user/cirrus/third_party/aws-sdk-cpp/build/aws-cpp-sdk-core/ \
	       -L/home/ec2-user/cirrus/third_party/aws-sdk-cpp/build/aws-cpp-sdk-s3 \
	       -L/home/ec2-user/kerberos/krb5-1.15.2/src/lib \
	       -L/home/ec2-user/keyutils/keyutils-1.5.10 \
	       -L/home/ec2-user/libevent/install_dir/lib/ \
	       -L/home/ec2-user/cirrus/third_party/gflags/

LDADD    = $(LINCLUDES) $(LIBS)
LDFLAGS  = -static-libgcc -Wl,--whole-archive \
	   -pthread -Wl,--no-whole-archive
CPPFLAGS = -static-libgcc -static-libstdc++ \
           -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
           -I$(TOP_DIR)/src \
           -I$(TOP_DIR)/third_party/flatbuffers/include \
           -I$(TOP_DIR)/third_party/eigen_source \
           -I$(TOP_DIR)/third_party/hiredis \
           -isystem $(TOP_DIR)/third_party/libcuckoo/ \
           -I $(MPI_HOME)/include/ -Itest_aws \
	   -I/home/ec2-user/cirrus/third_party/aws-sdk-cpp/aws-cpp-sdk-s3/include/ \
	   -I/home/ec2-user/cirrus/third_party/aws-sdk-cpp/aws-cpp-sdk-core/include/  \
           -I/home/ec2-user/cirrus/third_party/gflags/include/ \
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
