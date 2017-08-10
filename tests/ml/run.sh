#mpirun -hostfile ~/hosts -np 4 gdb -ex run -ex bt --args ~/parameter_server ~/criteo_fbox.cfg
#mpirun -hostfile ~/hosts -np 4 valgrind --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no --gen-suppressions=all ~/parameter_server ~/criteo_fbox.cfg
mpirun -hostfile ~/hosts -np 4 valgrind --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no ~/parameter_server ~/criteo_fbox.cfg
#mpirun -hostfile ~/hosts -np 4 ~/parameter_server ~/criteo_fbox.cfg
