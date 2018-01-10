all: outputs/main

NUM_PROCS=9
CC=/opt/openmpi/bin/mpic++
CC_FLAGS=-std=c++17 -O3 -funroll-loops -L/data/nathreya/cirrus/src/client/ -L/data/nathreya/cirrus/src/utils/  -I/data/nathreya/cirrus/third_party/libcuckoo/  -I/data/nathreya/cirrus/third_party/flatbuffers/include -I/data/nathreya/cirrus/ -I/data/nathreya/cirrus/src/ 
LD_FLAGS=-lpthread -lstdc++fs -lclient -lutils


setup:
	mkdir build outputs /nscratch/nathreya/sort_output_dir /nscratch/nathreya/stats

clean:
	rm -f build/* outputs/*
	find /nscratch/nathreya/sort_output_dir/ -maxdepth 1 -name '*.txt' -delete
	find /nscratch/nathreya/stats/ -maxdepth 1 -name '*.txt' -delete

data_clean:
	find /nscratch/nathreya/sort_output_dir/ -maxdepth 1 -name '*.txt' -delete
	find /nscratch/nathreya/stats/ -maxdepth 1 -name '*.txt' -delete

build/serialization.o: serialization.hpp serialization.cpp config_instance.hpp
	$(CC) $(CC_FLAGS) -c serialization.cpp -o build/serialization.o

build/read_lambda.o: read_lambda.hpp read_lambda.cpp config_instance.hpp
	$(CC) $(CC_FLAGS) -c read_lambda.cpp -o build/read_lambda.o

build/hash_lambda.o: config_instance.hpp hash_lambda.cpp hash_lambda.hpp
	$(CC) $(CC_FLAGS) -c hash_lambda.cpp -o build/hash_lambda.o

build/sort_lambda.o: config_instance.hpp sort_lambda.cpp sort_lambda.hpp
	$(CC) $(CC_FLAGS) -c sort_lambda.cpp -o build/sort_lambda.o

outputs/main: main.cpp build/serialization.o build/read_lambda.o build/hash_lambda.o build/sort_lambda.o
	$(CC) $(CC_FLAGS) -c main.cpp -o build/main.o
	$(CC) $(CC_FLAGS) build/*.o -o outputs/main $(LD_FLAGS)

run: outputs/main
	../../src/server/tcpservermain 50000 &
	mpirun -np $(NUM_PROCS) ./outputs/main
