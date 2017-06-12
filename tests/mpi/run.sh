#export LD_LIBRARY_PATH=~/
cp test_mpi ~/
mpirun -np 10 ~/test_mpi
#mpirun -x $LD_LIBRARY_PATH -hostfile ~/hosts2 -np 1 ~/test_mpi
