if DEBUG
AM_CFLAGS = -D_DEBUG_ -g -O2 -Wall
AM_CXXFLAGS = $(AM_CFLAGS)
else
AM_CFLAGS = -O2
AM_CXXFLAGS = $(AM_CFLAGS)
endif

#.SILENT:


bin_PROGRAMS = as-clx_ethip_ps

tinyxml_lib = $(top_srcdir)/tinyxml/src/.libs

as_clx_ethip_ps_SOURCES = as-clx_ethip_ps.cpp as-ethip_ps.h

as_clx_ethip_ps_LDADD = -lrt -ltinyxml $(top_srcdir)/motor/src/libas-util.la $(top_srcdir)/motor/src/libas-registro.la

as_clx_ethip_ps_LDFLAGS = -L$(tinyxml_lib) 

as_clx_ethip_ps_DEPENDENCIES = $(top_srcdir)/motor/src/libas-util.la $(top_srcdir)/motor/src/libas-registro.la

INCLUDES = -I$(top_srcdir)/tinyxml/include -I$(top_srcdir)/include -I../include
