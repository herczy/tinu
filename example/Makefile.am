bindir = $(prefix)/share/tinu-example
bin_PROGRAMS = example meta_example

example_SOURCES = example.c
example_CFLAGS = @LIBGLIB_CFLAGS@ @CFLAGS@
example_LDFLAGS = @LIBGLIB_LIBS@ -ldl
example_LDADD = ../lib/libtinu.la

meta_example_SOURCES = addmeta.c addmeta-metainfo.c
meta_example_CFLAGS = @LIBGLIB_CFLAGS@ @CFLAGS@
meta_example_LDFLAGS = @LIBGLIB_LIBS@ -ldl
meta_example_LDADD = ../lib/libtinu.la

if CXXWRAPPER
bin_PROGRAMS += cxxexample
cxxexample_SOURCES = cxxexample.cc
cxxexample_CXXFLAGS = $(example_CFLAGS)
cxxexample_LDFLAGS = $(example_LDFLAGS)
cxxexample_LDADD = $(example_LDADD) ../lib/libtinucxx.la
endif

addmeta-metainfo.c: addmeta.c
	../tools/tinu_create_metafile -o $@ addmeta.c
