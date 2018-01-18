TOP_DIR = $(top_srcdir)/../../

include $(TOP_DIR)/common.mk
AUTOMAKE_OPTIONS = foreign

CXX = g++
#CXX = $(MPI_HOME)/bin/mpic++

bin_PROGRAMS = parameter_server parameter_server_softmax

LIBS         =  -lclient -lauthentication -lutils \
		-levictionpolicies -lhiredis \
		-laws-cpp-sdk-s3 -laws-cpp-sdk-core \
		-lcurl -lssl -lcrypto -lz -ldl -lkrb5 -lk5crypto \
		-lall -lkeyutils -lhiredis -levent

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
	       -L/home/ec2-user/libevent/install_dir/lib/

LDADD    = $(LINCLUDES) $(LIBS)
LDFLAGS  = -static-libgcc  -static  -Wl,--whole-archive \
	   -lpthread -Wl,--no-whole-archive
CPPFLAGS = -static -static-libgcc -static-libstdc++ \
           -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
           -I$(TOP_DIR)/src \
           -I$(TOP_DIR)/third_party/flatbuffers/include \
           -I$(TOP_DIR)/third_party/eigen_source \
           -I$(TOP_DIR)/third_party/hiredis \
           -isystem $(TOP_DIR)/third_party/libcuckoo/ \
           -I $(MPI_HOME)/include/ -Itest_aws \
	   -I/home/ec2-user/cirrus/third_party/aws-sdk-cpp/aws-cpp-sdk-s3/include/ \
	   -I/home/ec2-user/cirrus/third_party/aws-sdk-cpp/aws-cpp-sdk-core/include/

CPP_SOURCES = InputReader.cpp Utils.cpp \
              Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp \
              ModelGradient.cpp MlUtils.cpp Configuration.cpp \
              Checksum.cpp SoftmaxModel.cpp Serializers.cpp \
              ErrorTask.cpp LogisticTask.cpp LoadingTask.cpp \
              PSTask.cpp PSSparseTask.cpp TasksWait.cpp MurmurHash3.cpp \
	      RedisIterator.cpp  S3Iterator.cpp PerformanceLambdaTask.cpp \
	      Redis.cpp S3.cpp LoadingTaskS3.cpp LogisticTaskS3.cpp SparseDataset.cpp \
	      LoadingSparseTaskS3.cpp ErrorSparseTask.cpp SparseLRModel.cpp LogisticSparseTaskS3.cpp \
	      S3SparseIterator.cpp

CPP_SOURCES_SOFTMAX = InputReader.cpp Utils.cpp \
              Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp \
              ModelGradient.cpp MlUtils.cpp Configuration.cpp \
              Checksum.cpp SoftmaxModel.cpp \
              Tasks_softmax.cpp Serializers.cpp  \
              Redis.cpp SparseDataset.cpp MurmurHash3.cpp

#PerformanceLambdaTask.cpp S3Iterator.cpp

#performance_lambda_SOURCES  = PerformanceLambdaTask.cpp $(CPP_SOURCES)
parameter_server_SOURCES  = parameter_server.cpp $(CPP_SOURCES)
parameter_server_softmax_SOURCES  = parameter_server_softmax.cpp \
				$(CPP_SOURCES_SOFTMAX)
