bindir = $(prefix)/share/tinu-example
bin_PROGRAMS = example

example_SOURCES = example.c
example_CFLAGS = -I../lib @LIBGLIB_CFLAGS@
example_LDFLAGS = @LIBGLIB_LIBS@ -ldl
example_LDADD = ../lib/libtinu.la
