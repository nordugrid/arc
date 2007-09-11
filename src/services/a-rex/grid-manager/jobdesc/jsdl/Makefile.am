noinst_LTLIBRARIES = libnamespace.la libjsdl.la

libjsdl_la_SOURCES=jsdl_job.cpp jsdl_job.h
nodist_libjsdl_la_SOURCES = jsdl_soapClient.cpp jsdl_soapC.cpp jsdl_soapStub.h stlvector.h
libjsdl_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_RSL_CFLAGS) $(GSOAP_CFLAGS)
libjsdl_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la 
#../../files/libfiles.la ../../misc/libmisc.la ../../url/liburl.la
libjsdl_la_LDFLAGS=$(GSOAP_LIBS)

libnamespace_la_SOURCES=namespace.cpp
libnamespace_la_CPPFLAGS=$(GSOAP_CFLAGS)
libnamespace_la_LDFLAGS=

BUILT_SOURCES = $(SOAPCPP2_OUTPUT) jsdl_soap.h

SOAPCPP2_OUTPUT = jsdl_soapClientLib.cpp jsdl_soapClient.cpp jsdl_soapC.cpp jsdl_soapStub.h

jsdl_soap.h: jsdl_complete.xsd jsdl_POSIXApplication.xsd jsdl_arc.xsd typemap.dat
	$(WSDL2H) -t typemap.dat -o jsdl_soap.h jsdl_POSIXApplication.xsd jsdl_arc.xsd jsdl_complete.xsd

$(SOAPCPP2_OUTPUT): jsdl_soap.h
	$(SOAPCPP2) -n -p jsdl_soap $<

CLEANFILES = $(SOAPCPP2_OUTPUT)

EXTRA_DIST = *.xsd typemap.dat stlvector.h
