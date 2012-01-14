#!/bin/sh
# This script is needed to setup build environment from checked out
# source tree. 
#

which libtoolize 2>/dev/null && LIBTOOLIZE=libtoolize || LIBTOOLIZE=glibtoolize

$LIBTOOLIZE -f
aclocal
autoheader
automake --foreign --add-missing
autoconf
