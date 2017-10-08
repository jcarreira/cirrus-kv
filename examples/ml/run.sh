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

(~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS} 0 2>&1 | tee output0)& 
(~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS} 1 2>&1 | tee output1)&
(~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS} 2 2>&1 | tee output2)&
(~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS} 3 2>&1 | tee output3)&
(~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS} 4 2>&1 | tee output4)&
(~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS} 5 2>&1 | tee output5)&

wait

#mpirun -output-filename ~/ps_output -hostfile ~/hosts -np ${TOTAL_WORKERS} ~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS}

# gdb
#mpirun -hostfile ~/hosts -np ${TOTAL_WORKERS} gdb -ex run -ex bt --args ~/parameter_server ~/criteo_fbox.cfg ${ML_WORKERS}
