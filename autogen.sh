#!/bin/sh
# This script is needed to setup build environment from checked out
# source tree. 
#

libtoolize -f
aclocal
autoheader
automake --foreign --add-missing
autoconf
