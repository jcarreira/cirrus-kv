#!/usr/bin/env python3

import sys
import subprocess
import time
import test_runner

# Set name of test to run
testPath = "./tests/object_store/exhaustion"
# Call script to run the test
test_runner.runExhaustionRDMA(testPath)
