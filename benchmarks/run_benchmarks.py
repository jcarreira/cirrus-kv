#!/usr/bin/env python3

import sys
import subprocess
import time

# NOTE: all pathnames start from the top directory where make check is run

# Launch the server in the background
print("Starting server.")
server = subprocess.Popen(["./src/server/bladeallocmain"])

# Sleep to give server time to start
print("Started server, sleeping.")
time.sleep(2)
print("Sleep finished, launching client.")

child = subprocess.Popen(["./benchmarks/1_2"], stdout=subprocess.PIPE)
streamdata = child.communicate()[0]
rc = child.returncode
print("Benchmark 1_2 complete")
server.kill()

if (rc != 0):
	sys.exit(rc)

print("Starting server.")
time.sleep(2)
server = subprocess.Popen(["./src/server/bladeallocmain"])

# Sleep to give server time to start
print("Started server, sleeping.")
time.sleep(2)
print("Sleep finished, launching benchmark 1_1.")

child = subprocess.Popen(["./benchmarks/1_1"], stdout=subprocess.PIPE)
streamdata = child.communicate()[0]
rc = child.returncode

server.kill()

sys.exit(rc)

