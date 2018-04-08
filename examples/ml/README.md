# Distrubuted ML 

## Running examples/ml from the command line:


Set the number of parameter servers, ports, and ips in config.h

```
#define NUM_PS <number of parameter servers>
// Make sure the csvs below have at least the same number of terms as NUM_PS
// You can use the same IP and different ports when using multiple PSs.
#define PS_IPS_LST "ip_addr1", "ip_addr2", "ip_addr3", etc... 
#define PS_PORTS_LST 1337, 1338, port_number, ...
```


Compile:

``` 
make -j4
```

Pass in 3 arguments through the terminal: config filename, the rank, number of workers, offset  

The rank argument:
```
1 for parameter server
2 for error task
3 and up for workers
```
The offset term is for designating which parameter server you want to start.  
For instance if you use offset 3, you will be starting a PS using the third ip in PS_IPS_LST
This term is ignored for all other tasks. 


To run a single parameter server with a single worker:
```
// starts a parameter server
./parameter_server <config_filename> 1 1 0 // starts a PS with the first IP and port
./parameter_server <config_filename> 1 2 0 // starts the error task, offset is ignored
./parameter_server <config_filename> 1 3 0 // starts the worker task, offset is ignored
```

To run 2 parameter servers with a single worker (remember to change the NUM_PS parameter in config.h and recompile)
```
./parameter_server <config_filename> 1 1 0 // starts a PS with the first IP and port
./parameter_server <config_filename> 1 1 1 // starts a PS with the second IP and port
./parameter_server <config_filename> 1 2 0 // starts the error task, offset is ignored
./parameter_server <config_filename> 1 3 0 // Starts the worker task, offset is ignored
```

