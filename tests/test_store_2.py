#!/usr/bin/env python3

import sys
import subprocess
import time

# NOTE: all pathnames start from the top directory where make check is run
testPath = "./tests/object_store/test_store_v2"

# Launch the server in the background
print("Starting server.")
# Sleep to give the server from the previous test time to close
time.sleep(1)
server = subprocess.Popen(["./src/server/bladeallocmain"])

# Sleep to give server time to start
print("Started server, sleeping.")
time.sleep(2)
print("Sleep finished, launching client.")

child = subprocess.Popen([testPath], stdout=subprocess.PIPE)
streamdata = child.communicate()[0]
rc = child.returncode

server.kill()

sys.exit(rc)
