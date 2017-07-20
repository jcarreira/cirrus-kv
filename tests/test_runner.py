#!/usr/bin/env python3

import sys
import subprocess
import time

# A function that will launch a test of a given name and return its exit
# status. Will automatically start and kill the server before and after
# the test.

# Change this to change the ip that the client connects to
ip = "127.0.0.1"

# NOTE: all pathnames start from the top directory where make check is run
def runTestTCP(testPath):
    # Launch the server in the background
    print("Starting server.")
    # Sleep to give the server from the previous test time to close
    time.sleep(1)
    server = subprocess.Popen(["./src/server/tcpservermain"])

    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath, "--tcp", ip], stdout=subprocess.PIPE)

    # Print the output from the child
    for line in child.stdout:
        print(line.decode(), end='')

    streamdata = child.communicate()[0]
    rc = child.returncode

    server.kill()
    sys.exit(rc)

def runTestRDMA(testPath):
    # Launch the server in the background
    print("Starting server.")
    # Sleep to give the server from the previous test time to close
    time.sleep(1)
    server = subprocess.Popen(["./src/server/bladeallocmain"])

    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath, "--rdma", ip], stdout=subprocess.PIPE)

    # Print the output from the child
    for line in child.stdout:
        print(line.decode(), end='')

    streamdata = child.communicate()[0]
    rc = child.returncode

    server.kill()
    sys.exit(rc)

def runExhaustionTCP(testPath):
    # Launch the server in the background
    print("Starting server.")
    # Sleep to give the server from the previous test time to close
    time.sleep(1)
    # 2 * 1024 * 1024 is the max pool. 2MB
    server = subprocess.Popen(["./src/server/tcpservermain", "2097152"])
    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath, "--tcp", ip], stdout=subprocess.PIPE)

    # Print the output from the child
    for line in child.stdout:
        print(line.decode(), end='')

    streamdata = child.communicate()[0]
    rc = child.returncode

    server.kill()
    sys.exit(rc)

def runExhaustionRDMA(testPath):
    # Launch the server in the background
    print("Starting server.")
    # Sleep to give the server from the previous test time to close
    time.sleep(1)
    server = subprocess.Popen(["./src/server/bladeallocmain", "1000"])
    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath, "--rdma", ip], stdout=subprocess.PIPE)

    # Print the output from the child
    for line in child.stdout:
        print(line.decode(), end='')

    streamdata = child.communicate()[0]
    rc = child.returncode

    server.kill()
    sys.exit(rc)
