if DEBUG
AM_CFLAGS = -D_DEBUG_ -g -Wall -O2
AM_CXXFLAGS = $(AM_CFLAGS)
else
AM_CFLAGS = -O2
AM_CXXFLAGS = $(AM_CFLAGS)
endif


library_includedir=$(includedir)/$(PACKAGE)
library_include_HEADERS= ../include/tinystr.h ../include/tinyxml.h

lib_LTLIBRARIES = libtinyxml.la

libtinyxml_la_SOURCES = tinyxml.cpp tinystr.cpp tinyxmlerror.cpp tinyxmlparser.cpp
libtinyxml_la_LIBADD = -lgcc


INCLUDES = -I$(top_srcdir)/include -I../include
libtinyxml_la_LDFLAGS = -L../lib


