if DEBUG
AM_CFLAGS = -D_DEBUG_ -g -Wall -O2
AM_CXXFLAGS = $(AM_CFLAGS)
else
AM_CFLAGS = -O2
AM_CXXFLAGS = $(AM_CFLAGS)
endif

bin_PROGRAMS = as-mbtcp as-emulmbtcp

tinyxml_lib = $(top_srcdir)/tinyxml/src/.libs

as_mbtcp_SOURCES = as-mbtcp.cpp modbus.c

as_mbtcp_LDADD = -lrt -ltinyxml $(top_srcdir)/motor/src/libas-util.la $(top_srcdir)/motor/src/libas-registro.la

as_mbtcp_LDFLAGS = -L$(tinyxml_lib) 

as_mbtcp_DEPENDENCIES = $(top_srcdir)/motor/src/libas-util.la $(top_srcdir)/motor/src/libas-registro.la

as_emulmbtcp_SOURCES = as-emulmbtcp.cpp modbus.c

as_emulmbtcp_LDADD = -lrt

INCLUDES = -I$(top_srcdir)/tinyxml/include -I$(top_srcdir)/include -I../include



