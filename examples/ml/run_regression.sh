# copy executable to the home directory
cp bug_regression ~/
#mpirun -hostfile ~/hosts -np 1 valgrind --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no ~/bug_regression ~/criteo_fbox.cfg
#mpirun -hostfile ~/hosts -np 1 gdb -ex run -ex bt --args ~/bug_regression ~/criteo_fbox.cfg
#mpirun -hostfile ~/hosts -np 1 ~/bug_regression ~/criteo_fbox.cfg

valgrind --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no ~/bug_regression ~/criteo_fbox_bug.cfg
#~/bug_regression ~/criteo_fbox_bug.cfg
#gdb -ex run -ex bt --args ~/bug_regression ~/criteo_fbox_bug.cfg
