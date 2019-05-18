if DEBUG
AM_CFLAGS = -D_DEBUG_ -g -Wall -O2
AM_CXXFLAGS = $(AM_CFLAGS)
else
AM_CFLAGS = -O2
AM_CXXFLAGS = $(AM_CFLAGS)
endif

AM_CXXFLAGS = $(AM_CFLAGS) -D_DATADIR_='"$(datadir)"'

library_includedir=$(includedir)/$(PACKAGE)

library_include_HEADERS= ../include/as-alarma.h \
	../include/as-historico.h \
	../include/as-tags.h \
	../include/as-shmtag.h \
	../include/as-shmalarma.h \
	../include/as-multicast.h \
	../include/as-thtag.h \
	../include/as-thalarma.h \
	../include/as-shmhistorico.h \
	../../include/as-registro.h \
	../../include/as-util.h \
	../../include/version.h \
	../../include/config.h


tinyxml_lib = $(top_srcdir)/tinyxml/src/.libs


INCLUDES = -I$(top_srcdir)/tinyxml/include -I$(top_srcdir)/include -I../include

#lib_LIBRARIES = libssutil.a libssregistro.a
#noinst_LTLIBRARIES = libas-util.la libas-registro.la

#libas_util_la_SOURCES = as-util.cpp as-util.h

#libas_registro_la_SOURCES = as-registro.cpp as-registro.h


lib_LTLIBRARIES = libas-util.la libas-registro.la libas-thtag.la libas-thalarma.la libas-multicast.la libas-historicos.la


libas_util_la_SOURCES = as-util.cpp as-util.h

libas_util_la_LDFLAGS = -version-info $(LIBRARY_VERSION) -release $(RELEASE)


libas_registro_la_SOURCES = as-registro.cpp as-registro.h

libas_registro_la_LDFLAGS = -version-info $(LIBRARY_VERSION) -release $(RELEASE)


libas_thtag_la_SOURCES = as-tags.cpp as-tags.h as-shmtag.cpp as-shmtag.h as-thtag.cpp

libas_thtag_la_LIBADD = libas-util.la

libas_thtag_la_LDFLAGS = -version-info $(LIBRARY_VERSION) -release $(RELEASE)


libas_thalarma_la_SOURCES = as-alarma.cpp as-alarma.h as-shmalarma.cpp as-shmalarma.h as-thalarma.cpp

libas_thalarma_la_LIBADD = libas-util.la

libas_thalarma_la_LDFLAGS = -version-info $(LIBRARY_VERSION) -release $(RELEASE)


libas_multicast_la_SOURCES = as-multicast.cpp as-multicast.h as-util.cpp

libas_multicast_la_DEPENDENCIES = libas-thtag.la libas-thalarma.la

libas_multicast_la_LDFLAGS = -version-info $(LIBRARY_VERSION) -release $(RELEASE)


libas_historicos_la_SOURCES = as-shmhistorico.cpp as-shmhistorico.h as-historico.cpp as-historico.h as-util.cpp

libas_historicos_la_DEPENDENCIES = libas-thtag.la libas-thalarma.la

libas_historicos_la_LDFLAGS = -version-info $(LIBRARY_VERSION) -release $(RELEASE)


bin_PROGRAMS = as-herramienta as-escaner as-datos as-cliente as-registrador as-bd as-escritor as-trazador


as_herramienta_SOURCES = as-herramienta.cpp

as_herramienta_DEPENDENCIES = libas-thtag.la libas-thalarma.la

as_herramienta_LDFLAGS =

as_herramienta_LDADD = -las-thtag -las-thalarma -lrt libas-util.la


as_escaner_SOURCES = as-escaner.cpp

as_escaner_DEPENDENCIES = libas-thtag.la libas-thalarma.la libas-util.la

as_escaner_LDFLAGS = -L$(tinyxml_lib)

as_escaner_LDADD = -las-thtag -las-thalarma -lrt -lpthread -ltinyxml libas-util.la


as_datos_SOURCES = as-datos.cpp

as_datos_DEPENDENCIES = libas-thtag.la libas-thalarma.la libas-util.la libas-multicast.la

as_datos_LDFLAGS = -L$(tinyxml_lib)

as_datos_LDADD = -las-thtag -las-thalarma -lrt -lpthread -ltinyxml -las-multicast


as_cliente_SOURCES = as-cliente.cpp

as_cliente_DEPENDENCIES = libas-thtag.la libas-thalarma.la libas-util.la libas-multicast.la

as_cliente_LDFLAGS = -L$(tinyxml_lib)

as_cliente_LDADD = -las-thtag -las-thalarma -lrt -lpthread -ltinyxml -las-multicast


as_registrador_SOURCES = as-registrador.cpp

as_registrador_DEPENDENCIES = libas-historicos.la libas-thtag.la libas-thalarma.la libas-util.la

as_registrador_LDFLAGS = -L$(tinyxml_lib)

as_registrador_LDADD = -las-historicos -las-thtag -las-thalarma -lrt -lpthread -ltinyxml -lcap


as_bd_SOURCES = as-bd.cpp

as_bd_DEPENDENCIES = libas-historicos.la libas-thtag.la libas-thalarma.la libas-util.la

as_bd_CPPFLAGS = $(PGSQL_CFLAGS)

as_bd_LDFLAGS = -L$(tinyxml_lib)

as_bd_LDADD = -las-historicos -las-thtag -las-thalarma -lrt -lpthread -ltinyxml $(PGSQL_LIBS)


as_escritor_SOURCES = as-escritor.cpp

as_escritor_DEPENDENCIES = libas-thtag.la libas-thalarma.la libas-util.la libas-registro.la

as_escritor_LDFLAGS = -L$(tinyxml_lib)

as_escritor_LDADD = -las-thtag -las-thalarma -lrt -lpthread -ltinyxml libas-util.la


as_trazador_SOURCES = as-trazador.cpp

as_trazador_DEPENDENCIES = libas-historicos.la libas-thtag.la libas-thalarma.la libas-util.la

as_trazador_CPPFLAGS = $(PGSQL_CFLAGS)

as_trazador_LDFLAGS = -L$(tinyxml_lib)

as_trazador_LDADD = -las-historicos -las-thtag -las-thalarma -lrt -lpthread -ltinyxml $(PGSQL_LIBS)



CLEANFILES	=	*~ *.save *.d ../include/*~ ../include/*.save


clean-generic:
	-test -z "$(CLEANFILES)" || rm -rf $(CLEANFILES)



