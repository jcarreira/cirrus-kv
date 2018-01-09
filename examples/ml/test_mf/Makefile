CXX=g++
CXXFLAGS=-Wall -ansi -O3 -std=c++14 -pthread
TOP_DIR=../../..
LIBFLAGS =-static-libgcc  -static \
	  -Wl,--whole-archive -lpthread -Wl,--no-whole-archive \
	  -L$(TOP_DIR)/src/utils/ \
	  -lutils
INCLUDES=-I.. \
	 -I../../../ \
	 -I../../../src/ \
	 -I$(TOP_DIR)/third_party/eigen_source

SOURCES=../Dataset.cpp ../ModelGradient.cpp ../Utils.cpp \
	../Matrix.cpp ../MlUtils.cpp ../Checksum.cpp ../InputReader.cpp \
	../SparseDataset.cpp ../MFModel.cpp  ../Model.cpp

PROJ1=test_mf
PROJ2=test_mf_threads

all: $(PROJ1) $(PROJ2)

$(PROJ1): $(PROJ1).cpp $(SOURCES)
	$(CXX) $(INCLUDES) $(CXXFLAGS) \
	  $(PROJ1).cpp $(SOURCES) \
	  -o $@ $(LIBFLAGS) 

$(PROJ2): $(PROJ2).cpp $(SOURCES)
	$(CXX) $(INCLUDES) $(CXXFLAGS) \
	  $(PROJ2).cpp $(SOURCES) \
	  -o $@ $(LIBFLAGS) 

clean:
	rm -rf a.out $(PROJ1) $(PROJ2)
