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

parameter_server_SOURCES  = parameter_server.cpp Input.cpp Utils.cpp \
                            Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp \
                            ModelGradient.cpp MlUtils.cpp Configuration.cpp
parameter_server_LDFLAGS  = -pthread
parameter_server_CPPFLAGS = -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
                	    -I$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
                            -I$(TOP_DIR)/src \
                	    -I$(TOP_DIR)/third_party/flatbuffers/include \
                	    -I$(TOP_DIR)/third_party/eigen_source
parameter_server_LDADD    = $(LINCLUDES) $(LIBS)

two_clients_test_SOURCES  = two_clients_test.cpp
two_clients_test_LDFLAGS  = -pthread
two_clients_test_CPPFLAGS = -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
                	    -I$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
                            -I$(TOP_DIR)/src \
                	    -I$(TOP_DIR)/third_party/flatbuffers/include
two_clients_test_LDADD    = $(LINCLUDES) $(LIBS)

bug_regression_SOURCES  = bug_regression.cpp Input.cpp Utils.cpp \
                          Dataset.cpp Matrix.cpp Model.cpp LRModel.cpp \
                          ModelGradient.cpp MlUtils.cpp Configuration.cpp
bug_regression_LDFLAGS  = -pthread
bug_regression_CPPFLAGS = -ggdb -I$(TOP_DIR) -I$(TOP_DIR)/examples/sparsehash/src/ \
                 	  -I$(TOP_DIR)/third_party/libcuckoo/cityhash-1.1.1/src/ -O3 \
                          -I$(TOP_DIR)/src \
                 	  -I$(TOP_DIR)/third_party/flatbuffers/include \
                 	  -I$(TOP_DIR)/third_party/eigen_source
bug_regression_LDADD    = $(LINCLUDES) $(LIBS)
