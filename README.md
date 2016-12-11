Sirius
==================================

[![Travis Build Status](https://travis-ci.org/jcarreira/ddc.svg?branch=master)](https://travis-ci.org/jcarreira/ddc)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/10708/badge.svg)](https://scan.coverity.com/projects/jcarreira-ddc)

Sirius is a system for memory management in disaggregated datacenter environments.

Requirements
================

This library has been tested on Ubuntu >= 14.04.

It has been tested with the following environment:
* Ubuntu 14.04
* g++ 6.2 (needs C++17)
* Boost
* autotools

You can install these with

    $ sudo apt-get update && sudo apt-get install build-essential autoconf libtool g++-6 libboost-all-dev

Building
==========

    $ sh boostrap.sh
    $ make
    $ make # there's an unsolved dependency
