#!/usr/bin/env python3

import sys
import subprocess
import time

# A function that will launch a test of a given name and return its exit
# status. Will automatically start and kill the server before and after
# the test.

# NOTE: all pathnames start from the top directory where make check is run
def runTest(testPath):
    # Launch the server in the background
    print("Starting server.")
    # Sleep to give the server from the previous test time to close
    time.sleep(1)
    server = subprocess.Popen(["./src/server/tcpservermain"])

    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath], stdout=subprocess.PIPE)
    streamdata = child.communicate()[0]
    rc = child.returncode

    server.kill()

    sys.exit(rc)
