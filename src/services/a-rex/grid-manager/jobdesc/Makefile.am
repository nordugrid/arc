if GSOAP_ENABLED
JSDLDIR=jsdl
JSDLLIB=jsdl/libjsdl.la
else
JSDLDIR=
JSDLLIB=
endif

if GLOBUS_ENABLED
RSLDIR=rsl
RSLLIB=rsl/librsl.la
else
RSLDIR=
RSLLIB=
endif

SUBDIRS = $(JSDLDIR) $(RSLDIR)

noinst_LTLIBRARIES = libjobdesc.la

libjobdesc_ladir=$(includedir)
libjobdesc_la_HEADERS=
libjobdesc_la_SOURCES=
libjobdesc_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libjobdesc_la_LDFLAGS=$(JSDLLIB) $(RSLLIB)
