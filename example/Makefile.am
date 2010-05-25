bindir = $(prefix)/share/tinu-example
bin_PROGRAMS = example

example_SOURCES = example.c
example_CFLAGS = -I../lib @LIBGLIB_CFLAGS@
example_LDFLAGS = @LIBGLIB_LIBS@ -ldl
example_LDADD = ../lib/libtinu.la

if CXXWRAPPER
bin_PROGRAMS += cxxexample
cxxexample_SOURCES = cxxexample.cc
cxxexample_CXXFLAGS = $(example_CFLAGS)
cxxexample_LDFLAGS = $(example_LDFLAGS)
cxxexample_LDADD = ../lib/libtinucxx.la
endif
