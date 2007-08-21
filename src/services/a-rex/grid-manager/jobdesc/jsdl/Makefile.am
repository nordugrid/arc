noinst_LTLIBRARIES = libnamespace.la libjsdl.la

libjsdl_ladir=$(includedir)
libjsdl_la_HEADERS=jsdl_job.h
libjsdl_la_SOURCES=jsdl_job.cpp
nodist_libjsdl_la_SOURCES = jsdl_soapClient.cpp jsdl_soapC.cpp
libjsdl_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_RSL_CFLAGS) $(GSOAP_CFLAGS)
libjsdl_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la 
#../../files/libfiles.la ../../misc/libmisc.la ../../url/liburl.la
libjsdl_la_LDFLAGS=$(GSOAP_LIBS)

libnamespace_ladir=$(includedir)
libnamespace_la_HEADERS=
libnamespace_la_SOURCES=namespace.cpp
libnamespace_la_CPPFLAGS=$(GSOAP_CFLAGS)
libnamespace_la_LDFLAGS=

#nodist_noinst_HEADERS = jsdl_soapStub.h

jsdl_job.lo: jsdl_soapStub.h

jsdl_soap.h: jsdl_complete.xsd jsdl_POSIXApplication.xsd jsdl_arc.xsd typemap.dat
	$(WSDL2H) -t typemap.dat -o jsdl_soap.h jsdl_POSIXApplication.xsd jsdl_arc.xsd jsdl_complete.xsd

jsdl_soapClientLib.cpp jsdl_soapClient.cpp jsdl_soapC.cpp jsdl_soapStub.h: jsdl_soap.h stlvector.h
	$(SOAPCPP2) -n -p jsdl_soap jsdl_soap.h

CLEANFILES = jsdl_soap*
