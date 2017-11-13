all:
	mpic++ -L/data/joao/cirrus/src/client/ -L/data/joao/cirrus/src/utils/  -I/data/joao/cirrus/third_party/libcuckoo/  -I/data/joao/cirrus/third_party/flatbuffers/include -I/data/joao/cirrus/ -I/data/joao/cirrus/src/ -std=c++14 test_mpi.cpp -o test_mpi -lclient -lutils

run:
	mpirun -np 4 ./test_mpi

clean:
	rm core -rf *.o test_mpi
