import os
import subprocess
import threading
from time import sleep
import time
import sys

CHARS_TO_PRINT = 100
TIMEOUT_TIME   = 60

def launch_ps(proc_num):

    command = './main {}'.format(proc_num)

    print("Command: ", command)
    process = subprocess.Popen(command.split(), stdout=subprocess.PIPE)

    now = time.time()
    output = ""
    for c in iter(lambda: process.stdout.read(CHARS_TO_PRINT), ''):
        output = c
        for c in iter(lambda: process.stdout.read(1), ''):
            output += c
            if c == '\n':
               print(output)
               output = ""
               break
        
    for c in iter(lambda: process.stdout.read(1), ''):
        output += c
    print(output)

    return ''.join(output)

def handler(event, context):
    results = []

    print("event: ", event)
    proc_num = event['proc_num']

    print("proc_num:", proc_num)
    results = launch_ps(proc_num)

    return []
