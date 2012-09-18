rootincludedir = $(prefix)/include
includedir = $(rootincludedir)/tinu
pkglibdir = $(prefix)/lib

rootinclude_HEADERS = tinu.h
include_HEADERS = tinu/backtrace.h \
                  tinu/config.h \
                  tinu/leakwatch.h \
                  tinu/log.h \
                  tinu/main.h \
                  tinu/message.h \
                  tinu/meta.h \
                  tinu/test.h \
                  tinu/utils.h \
                  tinu/clist.h \
                  tinu/statistics.h \
                  tinu/names.h \
                  tinu/reporting.h

lib_LTLIBRARIES = libtinu.la
libtinu_la_SOURCES = backtrace.c \
                     leakwatch.c \
                     log.c \
                     main.c \
                     message.c \
                     meta.c \
                     test.c \
                     utils.c \
                     clist.c \
                     statistics.c \
                     names.c \
                     reporting.c \
                     report-standard.c \
                     report-external.c
libtinu_la_CFLAGS = -Itinu -Wall @LIBGLIB_CFLAGS@ @CFLAGS@
libtinu_la_LDFLAGS = -version-info 0:0:0 @LIBGLIB_LIBS@ @LDFLAGS@

if ELFDEBUG
libtinu_la_SOURCES += dwarf.c
include_HEADERS += tinu/dwarf.h
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
include_HEADERS += tinu/cxxwrap.h
endif
