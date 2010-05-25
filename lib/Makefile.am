includedir = $(prefix)/include/tinu
pkglibdir = $(prefix)/lib

lib_LTLIBRARIES = libtinu.la
libtinu_la_SOURCES = backtrace.c leakwatch.c log.c main.c message.c  test.c tinu tinu.h tinu.pc.in utils.c
libtinu_la_CFLAGS = -Itinu -Wall @LIBGLIB_CFLAGS@ @CFLAGS@
libtinu_la_LDFLAGS = -version-info 0:0:0 @LIBGLIB_LIBS@ @LIBS@

if ELFDEBUG
libtinu_la_SOURCES += dwarf.c
libtinu_la_LDFLAGS += -lelf -ldwarf
if COREDUMPER
libtinu_la_CFLAGS += -I../coredumper
libtinu_la_LDFLAGS += ../coredumper/libcoredumper.la
endif
endif

if CXXWRAPPER
lib_LTLIBRARIES += libtinucxx.la
libtinucxx_la_SOURCES = cxxwrap.cc
libtinucxx_la_CXXFLAGS = $(libtinu_la_CFLAGS)
libtinucxx_la_LDFLAGS = $(libtinu_la_LDFLAGS) libtinu.la
endif
