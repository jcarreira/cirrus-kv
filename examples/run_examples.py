#!/usr/bin/env python3

import sys
import subprocess
import time

# NOTE: all pathnames start from the top directory where make benchmark is run

examples = [["./examples/graphs/page_rank/pr", "examples/graphs/data/graph_1"],
        ["./examples/graphs/k_cores/kc", "examples/graphs/data/graph_1"]]

server_name =  ["./src/server/tcpservermain"]

for example in examples:

    # Launch the server in the background
    print("Starting server.")
    server = subprocess.Popen(server_name)

    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    sys.stdout.write("Example " + example[0] + "...")
    child = subprocess.Popen(example, stdout=subprocess.PIPE)

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
