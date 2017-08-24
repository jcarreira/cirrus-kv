#!/usr/bin/bash

echo "Make sure server is running"

EXEC=parameter_server_softmax

cp ${EXEC} ~/${EXEC}
cp configs/mnist_fbox.cfg ~/


# valgrind
#mpirun -hostfile ~/hosts -np 5 valgrind --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no --gen-suppressions=all ~/${EXEC} ~/mnist_fbox.cfg
#mpirun -hostfile ~/hosts -np 5 valgrind --log-file=valgrind_output --suppressions=/home/eecs/joao/vg_suppressions_cirrus --error-limit=no ~/${EXEC} ~/mnist_fbox.cfg

# plain

SUPPORT_WORKERS=4
ML_WORKERS=2
TOTAL_WORKERS=$((${SUPPORT_WORKERS} + ${ML_WORKERS}))

echo "Running ${EXEC}. MPI Support workers:${SUPPORT_WORKERS} Ml_workers:${ML_WORKERS}"

mpirun -output-filename ~/ps_output -hostfile ~/hosts -np ${TOTAL_WORKERS} ~/${EXEC} ~/mnist_fbox.cfg ${ML_WORKERS}

# gdb
#mpirun -hostfile ~/hosts -np ${TOTAL_WORKERS} gdb -ex run -ex 'set height 0' -ex bt --args ~/${EXEC} ~/mnist_fbox.cfg ${ML_WORKERS}
