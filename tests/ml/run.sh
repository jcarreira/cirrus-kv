cp parameter_server ~/parameter_server

# gdb
#mpirun -hostfile ~/hosts -np 5 gdb -ex run -ex bt --args ~/parameter_server ~/criteo_fbox.cfg

# valgrind
#mpirun -hostfile ~/hosts -np 5 valgrind --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no --gen-suppressions=all ~/parameter_server ~/criteo_fbox.cfg
#mpirun -hostfile ~/hosts -np 5 valgrind --log-file=valgrind_output --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no ~/parameter_server ~/criteo_fbox.cfg

# plain
mpirun -output-filename ~/ps_output -hostfile ~/hosts -np 5 ~/parameter_server ~/criteo_fbox.cfg
