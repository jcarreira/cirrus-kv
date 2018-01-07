#!/usr/bin/env python3

import sys
import subprocess
import time
import test_runner

# Set name of test to run
testPath = "./tests/object_store/test_shards"
# Call script to run the test
num_shards = 2
test_runner.runTestShardedTCP(2, testPath)
