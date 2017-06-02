#export LD_LIBRARY_PATH=~/
export LD_LIBRARY_PATH=/data/joao/ddc/third_party/libcuckoo/cityhash-1.1.1/src/.libs
cp test_mpi ~/
mpirun -x $LD_LIBRARY_PATH -np 10 ~/test_mpi
#mpirun -x $LD_LIBRARY_PATH -hostfile ~/hosts2 -np 1 ~/test_mpi
