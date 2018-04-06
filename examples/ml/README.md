# Running examples/ml from the command line:


Set the number of parameter servers and port ip in config.h

Compile:

Pass in 3 arguments through the terminal: config filename, the rank, number of workers  

The rank argument:
1 for parameter server
2 for error task
3 and up for workersTo run a single parameter server with a single worker:
// starts a parameter server
./parameter_server <config_filename> 1 1// starts a worker
./parameter_server <config_filename> 3 1// starts the error task
./parameter_server <config_filename> 2 1
