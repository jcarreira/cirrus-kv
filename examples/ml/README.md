# Distrubuted ML 

## Running examples/ml from the command line:


Set the number of parameter servers and port ip in config.h

Compile:

``` 
make -j4
```

Pass in 3 arguments through the terminal: config filename, the rank, number of workers, offset  

The rank argument:
1 for parameter server
2 for error task
3 and up for workers

The offset term is for starting a parameter server on a different port number, where the new port number is IP_PORT + offset. 
This term is ignored for all other tasks. 

To run a single parameter server with a single worker:
// starts a parameter server
./parameter_server <config_filename> 1 1 0 // starts a worker at IP_PS, IP_PORT+0
./parameter_server <config_filename> 1 2 0 // starts the error task, offset is ignored
./parameter_server <config_filename> 1 3 0 // starts the worker task, offset is ignored


To run 2 parameter servers with a single worker
./parameter_server <config_filename> 1 1 0 // starts a worker at IP_PS, IP_PORT+0
./parameter_server <config_filename> 1 1 1 // starts a worker at IP_PS, IP_PORT+1
./parameter_server <config_filename> 1 2 0 // starts the error task, offset is ignored
./parameter_server <config_filename> 1 3 0 // Starts the worker task, offset is ignored
