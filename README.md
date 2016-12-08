Sirius
==================================

[![Travis Build Status](https://travis-ci.org/jcarreira/ddc.svg?branch=master)](https://travis-ci.org/jcarreira/ddc)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/10708/badge.svg)](https://scan.coverity.com/projects/jcarreira-ddc)

Sirius is a system for memory management in disaggregated datacenter environments.

Requirements
================

This library has been tested on Ubuntu >= 14.04.

It compiles with g++ 6.2 and requires the C++17 features to compile.
Building the library requires the autotools. Install them on Ubuntu

    $ sudo apt-get update && sudo apt-get install build-essential autoconf libtool

Building
==========

    $ sh boostrap.sh
    $ make
    $ make # there's an unsolved dependency
