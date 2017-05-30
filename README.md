Cirrus
==================================

[![Travis Build Status](https://travis-ci.org/jcarreira/ddc.svg?branch=master)](https://travis-ci.org/jcarreira/ddc)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/10708/badge.svg)](https://scan.coverity.com/projects/jcarreira-ddc)

Cirrus is a system for memory management in disaggregated datacenter environments.

Requirements
============

This library has been tested on Ubuntu >= 14.04.

It has been tested with the following environment:
* Ubuntu 14.04
* g++ 6.2 (needs C++17)
* Boost
* autotools
* Mellanox OFED 3.4 (requires Mellanox drivers)
* cmake

You can install these with

    $ sudo apt-get update && sudo apt-get install build-essential autoconf libtool g++-6 libboost-all-dev cmake

Make sure the compilation is done with g++-6. *update-alternatives* can be used:

    $ sudo apt-get install g++-6
    $ sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-6 100
    $ sudo update-alternatives --config g++ # choose g++-6

Building
=========

    $ ./boostrap.sh
    $ make


Running Tests
=============

Client, server example:

    $ ./ddc/src/server/bladeallocmain&
    $ ./ddc/src/client/clientmain

Benchmarks
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
