Sirius
==================================

[![Travis Build Status](https://travis-ci.org/jcarreira/ddc.svg?branch=master)](https://travis-ci.org/jcarreira/ddc)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/10708/badge.svg)](https://scan.coverity.com/projects/jcarreira-ddc)

Sirius is a system for memory management in disaggregated datacenter environments.

Requirements
============

This library has been tested on Ubuntu >= 14.04.

It has been tested with the following environment:
* Ubuntu 14.04
* g++ 6.2 (needs C++17)
* Boost
* autotools

You can install these with

    $ sudo apt-get update && sudo apt-get install build-essential autoconf libtool g++-6 libboost-all-dev
    
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

1. Single node burst of 1000 messages (synchronous)
```
    Avg latency: 5.13us
    SD: 1.49us
    99%: 6us
```
