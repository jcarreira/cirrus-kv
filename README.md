Cirrus
==================================

[![Travis Build Status](https://travis-ci.org/jcarreira/ddc.svg?branch=master)](https://travis-ci.org/jcarreira/ddc)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/10708/badge.svg)](https://scan.coverity.com/projects/jcarreira-ddc)

Cirrus is a remote data access system for interacting with disaggregated memory from uInstances in a performant fashion.

Requirements
============

This library has been tested on Ubuntu >= 14.04 as well as MacOS 10.12.5. Additional Mac requirements and instructions are listed below.

It has been tested with the following environment / dependencies:
* Ubuntu 14.04
* g++ 5.4
* Boost
* autotools
* Mellanox OFED 3.4 (optional)
* cmake
* cpplint
* snappy
* bzip2
* zlib

You can install these with

    $ sudo apt-get update && sudo apt-get install build-essential autoconf libtool g++-6 libboost-all-dev cmake libsnappy-dev zlib1g-dev libbz2-dev && sudo pip install cpplint

MacOS Requirements
============
Building on MacOS requires the installation of gettext, boost and wget. Please ensure that automake, autoconf, xcode command line tools, and gcc/g++ are installed. gcc/g++ can be installed using macports, and the `port select` command allows you to set the new version of gcc/g++ as the one you want to use. The remaining programs can be installed using homebrew. cpplint can be installed via pip.

gettext can be installed as follows using homebrew:

    $ brew install gettext
    $ brew link --force gettext

To install wget do:

    $ brew install wget

To install gcc/g++ do:

    $ port install gcc5
    $ port select --list gcc

To install boost do:

    $ sudo port install boost

To install cpplint do:

    $ pip install cpplint

Building
=========

    $ ./boostrap.sh
    $ make


Running Tests
=============

To run tests, run the following command from the top level of the project:

    $ make check

To create additional tests, add them to the TESTS variable in the top level Makefile.am . Tests are currently located in the tests directory. To change the ip address (necessary for RDMA), change the address in `tests/test_runner.py`.


Benchmarks
=============

To run benchmarks execute the following command from the top of the project directory

    $ make benchmark

This will leave log files for each benchmark run in the top directory. To add additional benchmarks, modify the script `run_benchmarks.py`, located in the benchmarks directory. The benchmarks are currently set to run locally, but may be set to run using a remote server by manually changing the ip address in the benchmark files. However, this then makes it so that the benchmarks must be manually launched from the command line after starting the server remotely. Additionally, the log files will be left in the benchmarks directory.


Benchmark Results (outdated)
=============

* Single node burst of 128 byte put (synchronous) - latencies
```
    msg/s: 166427
    min: 5
    avg: 5.34385
    max: 93
    sd: 0.993202
    99%: 6
```
* Single node burst of 128 byte put (async) - latencies
```
    min: 50 us
    avg: 261.7 us
    max: 460 us
    sd: 118.149 us
    99%: 459 us
```
* Single node contention 10 source clients 128 byte put (sync)
```
    Average (us) per msg: 16
    MSG/s: 61715.9
```
```
    Average (us) per msg: 9 # with 6 clients
    MSG/s: 105298
```
* Single node contention 6 source clients 128 byte put (async)
```
    Average (us) per msg: 11
    MSG/s: 87242.9
```

Static analysis with Coverity
=============

Download coverity scripts (e.g., cov-analysis-linux64-8.5.0.5.tar.gz)

~~~
tar -xof cov-analysis-linux64-8.5.0.5.tar.gz
~~~

Make sure all configure.ac are setup to use C++11
~~~
cov-build --dir cov-int make -j 10

tar czvf cirrus_cov.tgz cov-int
~~~

Upload file to coverity website

Cirrus on Lambdas
=============

Checkout the cirrus_on_lambdas repo
```
cp ../cirrus/examples/ml/parameter_server .
sh bundle.sh
./update_lambda.sh
./invoke_lambda.sh
```


