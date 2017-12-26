CXX=g++
CXXFLAGS=-Wall -ansi -O3 -std=c++14
TOP_DIR=../../..
LIBFLAGS =-static-libgcc  -static \
	  -Wl,--whole-archive -lpthread -Wl,--no-whole-archive \
	  -L$(TOP_DIR)/src/utils/ \
	  -lutils
INCLUDES=-I.. \
	 -I../../../ \
	 -I../../../src/ \
	 -I$(TOP_DIR)/third_party/eigen_source

PROJ1=check_correctness_cirrus_gd
PROJ2=check_correctness_cirrus_sgd
PROJ3=check_correctness_cirrus_gd_sparse

all: $(PROJ1) $(PROJ2) $(PROJ3)

$(PROJ1): $(PROJ1).cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) \
	  $(PROJ1).cpp \
	  ../Dataset.cpp ../LRModel.cpp ../ModelGradient.cpp ../Utils.cpp \
	  ../Matrix.cpp ../MlUtils.cpp ../Checksum.cpp ../InputReader.cpp \
	  -o $@ $(LIBFLAGS) 

$(PROJ2): $(PROJ2).cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) \
	  $(PROJ2).cpp \
	  ../Dataset.cpp ../LRModel.cpp ../ModelGradient.cpp ../Utils.cpp \
	  ../Matrix.cpp ../MlUtils.cpp ../Checksum.cpp ../InputReader.cpp \
	  -o $@ $(LIBFLAGS) 

$(PROJ3): $(PROJ3).cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) \
	  $(PROJ1).cpp \
	  ../Dataset.cpp ../LRModel.cpp ../ModelGradient.cpp ../Utils.cpp \
	  ../Matrix.cpp ../MlUtils.cpp ../Checksum.cpp ../InputReader.cpp \
	  -o $@ $(LIBFLAGS)
clean:
	rm -rf a.out $(PROJ1) $(PROJ2) $(PROJ3)
