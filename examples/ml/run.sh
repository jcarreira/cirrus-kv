#!/usr/bin/bash

echo "Make sure server is running"

cp parameter_server ~/parameter_server
cp configs/criteo_fbox.cfg ~/


# valgrind
#mpirun -hostfile ~/hosts -np 5 valgrind --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no --gen-suppressions=all ~/parameter_server ~/criteo_fbox.cfg
#mpirun -hostfile ~/hosts -np 5 valgrind --log-file=valgrind_output --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no ~/parameter_server ~/criteo_fbox.cfg

# plain

SUPPORT_WORKERS=4
ML_WORKERS=2
TOTAL_WORKERS=$((${SUPPORT_WORKERS} + ${ML_WORKERS}))

echo "Running MPI Support workers:${SUPPORT_WORKERS} Ml_workers:${ML_WORKERS}"

mpirun -output-filename ~/ps_output -hostfile ~/hosts -np ${TOTAL_WORKERS} ~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS}

# gdb
#mpirun -hostfile ~/hosts -np ${TOTAL_WORKERS} gdb -ex run -ex bt --args ~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS}
