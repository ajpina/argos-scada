if DEBUG
AM_CFLAGS = -D_DEBUG_ -g -Wall -O2
AM_CXXFLAGS = $(AM_CFLAGS)
else
AM_CFLAGS = -O2
AM_CXXFLAGS = $(AM_CFLAGS)
endif

bin_PROGRAMS = as-clx_ethip

tinyxml_lib = $(top_srcdir)/tinyxml/src/.libs

as_clx_ethip_SOURCES = as-clx_ethip.cpp TuxEip.h AB.c ErrCodes.c SendData.c CIP_IOI.c Ethernet_IP.c LGX.c MR.c CM.c PLC.c

as_clx_ethip_LDADD = -lrt -ltinyxml $(top_srcdir)/motor/src/libas-util.la $(top_srcdir)/motor/src/libas-registro.la

as_clx_ethip_LDFLAGS = -L$(tinyxml_lib) 

as_clx_ethip_DEPENDENCIES = $(top_srcdir)/motor/src/libas-util.la $(top_srcdir)/motor/src/libas-registro.la

INCLUDES = -I$(top_srcdir)/tinyxml/include -I$(top_srcdir)/include -I../include



