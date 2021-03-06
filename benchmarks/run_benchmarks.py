#!/usr/bin/env python3

import sys
import subprocess
import time

# NOTE: all pathnames start from the top directory where make benchmark is run

benchmarks = [["./benchmarks/1_3"], ["./benchmarks/1_1"],
    ["./benchmarks/iterator_benchmark"], ["./benchmarks/cache_benchmark"],
    ["./benchmarks/1_2"], ["./benchmarks/outstanding_requests"],
    ["./benchmarks/throughput"], ["./benchmarks/remove_bulk"]]

server_name =  ["./src/server/tcpservermain"]

for benchmark in benchmarks:

    # Launch the server in the background
    print("Starting server.")
    server = subprocess.Popen(server_name)

    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    sys.stdout.write("Benchmark " + benchmark[0] + "...")
    child = subprocess.Popen(benchmark, stdout=subprocess.PIPE)

    # Uncomment below two lines to print the benchmark's output

    # for line in child.stdout:
    #     print(line.decode(), end='')

    streamdata = child.communicate()[0]
    rc = child.returncode
    server.kill()

    #give server time to die
    time.sleep(1)
    if (rc != 0):
        print(" Failed")
        sys.exit(rc)
    else:
        print(" Success")
