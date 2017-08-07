Cirrus
==================================

[![Travis Build Status](https://travis-ci.org/jcarreira/ddc.svg?branch=master)](https://travis-ci.org/jcarreira/ddc)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/10708/badge.svg)](https://scan.coverity.com/projects/jcarreira-ddc)

Cirrus is a remote data access system for interacting with disaggregated memory from uInstances in a performant fashion.

Requirements
============

This library has been tested on Ubuntu >= 14.04 as well as MacOS 10.12.5. Additional Mac requirements and instructions are listed below.

It has been tested with the following environment:
* Ubuntu 14.04
* g++ 5.4 (needs C++17)
* Boost
* autotools
* Mellanox OFED 3.4 (requires Mellanox drivers)
* cmake
* cpplint

You can install these with

    $ sudo apt-get update && sudo apt-get install build-essential autoconf libtool g++-6 libboost-all-dev cmake && sudo pip install cpplint

Make sure the compilation is done with g++-6. *update-alternatives* can be used:

    $ sudo apt-get install g++-6
    $ sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-6 100
    $ sudo update-alternatives --config g++ # choose g++-6

MacOS Requirements
============
Building on MacOS has slightly different requirements than on linux. Namely, it requires the installation of gettext, which provides some of the macros used in the configure file. Otherwise, simply ensure that automake, autoconf, and a high enough version of gcc/g++ are installed. Additionally, make sure that xcode command line tools are installed, as this should provide make and other necessary programs. gcc/g++ can be installed using macports, and the `port select` command allows you to set the new version of gcc/g++ as the one you want to use. The remaining programs can be installed using homebrew. cpplint can still be installed via pip.

gettext can be installed as follows using homebrew. Read the warning before running the link command:

    $ brew install gettext
    $ brew link --force gettext

To install gcc/g++ you may have to do:

    $ port install gcc5
    $ port select --list gcc

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
