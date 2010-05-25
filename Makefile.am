if COREDUMPER
SUBDIRS = coredumper
else
SUBDIRS =
endif
SUBDIRS += lib example

EXTRA_DIST = tinu.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = tinu.pc

if CXXWRAPPER
EXTRA_DIST += tinucxx.pc
pkgconfig_DATA += tinucxx.pc
endif
