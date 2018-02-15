CXX=g++
CXXFLAGS=-Wall -ansi -O3 -std=c++14 -ggdb
TOP_DIR=../../..
LIBFLAGS =-static-libgcc  -static \
	  -Wl,--whole-archive -lpthread -Wl,--no-whole-archive \
	  -L$(TOP_DIR)/src/utils/ \
	  -L/home/ec2-user/cirrus/third_party/aws-sdk-cpp/build/aws-cpp-sdk-core/ \
	  -L/home/ec2-user/cirrus/third_party/aws-sdk-cpp/build/aws-cpp-sdk-s3 \
	  -L/home/ec2-user/kerberos/krb5-1.15.2/src/lib \
	  -L/home/ec2-user/keyutils/keyutils-1.5.10 \
	  -laws-cpp-sdk-s3 -laws-cpp-sdk-core \
	  -lutils -lcurl -lssl -lcrypto -lz -ldl -lkrb5 -lk5crypto \
	  -lall -lkeyutils -lhiredis

INCLUDES=-I.. \
	 -I../../../ \
	 -I../../../src/ \
	 -I/home/ec2-user/cirrus/third_party/aws-sdk-cpp/aws-cpp-sdk-s3/include/ \
	 -I/home/ec2-user/cirrus/third_party/aws-sdk-cpp/aws-cpp-sdk-core/include/ \
	 -I$(TOP_DIR)/third_party/eigen_source \
	 -I$(TOP_DIR)/third_party/hiredis

PROJ1=lr_vw
PROJ2=lr_vw_parallel

all: $(PROJ1) $(PROJ2)

lr_vw: $(PROJ1).cpp hash.h
	$(CXX) $(INCLUDES) $(CXXFLAGS) \
	$(PROJ1).cpp ../MurmurHash3.cpp \
	-o $@ $(LIBFLAGS) 

lr_vw_parallel: $(PROJ2).cpp hash.h
	$(CXX) $(INCLUDES) $(CXXFLAGS) \
	$(PROJ2).cpp ../MurmurHash3.cpp \
	-o $@ $(LIBFLAGS) 

clean:
	rm -rf a.out $(PROJ1) $(PROJ2)
