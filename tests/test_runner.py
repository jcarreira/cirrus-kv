#!/usr/bin/env python3

import sys
import subprocess
import time
import shutil
import os

# Change this to change the ip that the client connects to
MB = (1024 * 1024)
half_gig = 512 # in MB
        
storage_path = "/tmp/cirrus_storage"

def remove_nonvolatile_storage(storage_path):
    print("Removing: ", storage_path)
    shutil.rmtree(storage_path, True)

# check whether to use nv storage instead of memory
def use_storage():
    ret = os.getenv('CIRRUS_TEST_STORAGE')
    return ret == "1"

def get_test_ip():
    return os.getenv('CIRRUS_SERVER_TEST_IP', "127.0.0.1")

# A function that will launch a test of a given name and return its exit
# status. Will automatically start and kill the server before and after
# the test.
# NOTE: all pathnames start from the top directory where make check is run
def runTestTCP(testPath):

    # Launch the server in the background
    print("Running test", testPath)
    # Sleep to give the server from the previous test time to close
    time.sleep(1)

    if use_storage():
        print("Using storage backend")
        remove_nonvolatile_storage(storage_path);
        server = subprocess.Popen(
                ["./src/server/tcpservermain",
                 "12345",
                 str(half_gig),
                 "Storage", storage_path])
    else:
        print("Using memory backend")
        server = subprocess.Popen(["./src/server/tcpservermain"])

    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(3)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath, "--tcp", get_test_ip()],
                             stdout=subprocess.PIPE)

    # Print the output from the child
    for line in child.stdout:
        print(line.decode(), end = '')

    streamdata = child.communicate()[0]
    rc = child.returncode

    server.kill()
    if use_storage():
        remove_nonvolatile_storage(storage_path);
    sys.exit(rc)

# A function that will launch a test of a given name and return its exit
# status. Will automatically start and kill the server before and after
# the test.
# This test is used to run multiple Cirrus servers
# NOTE: all pathnames start from the top directory where make check is run
def runTestShardsTCP(num_shards, testPath):
    # Launch the server in the background
    print("Running test", testPath, " with #shards: ", num_shards)
    # Sleep to give the server from the previous test time to close
    time.sleep(1)

    serves = []

    for i in range(0, num_shards):
        if use_storage():
            print("Using storage backend")

            store_path = storage_path + str(i)

            remove_nonvolatile_storage(store_path);
            servers.append(subprocess.Popen(
                    ["./src/server/tcpservermain",
                     str(12345 + i),
                     str(half_gig),
                     "Storage", store_path]))
        else:
            print("Using memory backend")
            servers.append(subprocess.Popen(
                    ["./src/server/tcpservermain",
                     str(12345 + i)]))

    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(3)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath, "--tcp",
                              get_test_ip(),
                              "--shards", str(num_shards)],
                              stdout=subprocess.PIPE)

    # Print the output from the child
    for line in child.stdout:
        print(line.decode(), end = '')

    streamdata = child.communicate()[0]
    rc = child.returncode

    for server in servers:
        server.kill()

    if use_storage():
        for i in range(0, num_shards):
            store_path = storage_path + str(i)
            remove_nonvolatile_storage(store_path);
    sys.exit(rc)


def runTestRDMA(testPath):
    # Launch the server in the background
    print("Running test", testPath)
    # Sleep to give the server from the previous test time to close
    time.sleep(1)

    size = 20 * half_gig
    server = subprocess.Popen(["./src/server/bladeallocmain", str(size)])

    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath, "--rdma", get_test_ip()],
                             stdout=subprocess.PIPE)

    # Print the output from the child
    for line in child.stdout:
        print(line.decode(), end='')

    streamdata = child.communicate()[0]
    rc = child.returncode

    server.kill()
    sys.exit(rc)

def runExhaustionTCP(testPath):
    # Launch the server in the background
    print("Running test", testPath)
    # Sleep to give the server from the previous test time to close
    time.sleep(1)
    
    limit_size = 20
    server = subprocess.Popen(["./src/server/tcpservermain",
                               "12345",
                               str(limit_size)])
    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath, "--tcp", get_test_ip()],
                             stdout=subprocess.PIPE)

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

    limit_size = 20
    server = subprocess.Popen(["./src/server/bladeallocmain", str(limit_size)])
    # Sleep to give server time to start
    print("Started server, sleeping.")
    time.sleep(2)
    print("Sleep finished, launching client.")

    child = subprocess.Popen([testPath, "--rdma", get_test_ip()],
                             stdout=subprocess.PIPE)

    # Print the output from the child
    for line in child.stdout:
        print(line.decode(), end='')

    streamdata = child.communicate()[0]
    rc = child.returncode

    server.kill()
    sys.exit(rc)
