noinst_LTLIBRARIES = libjsdl.la

libjsdl_ladir=$(includedir)
libjsdl_la_HEADERS=jsdl_job.h
libjsdl_la_SOURCES=jsdl_job.cpp namespace.cpp
nodist_libjsdl_la_SOURCES = jsdl_soapC.cpp
libjsdl_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_CFLAGS) $(GSOAP_CFLAGS)
libjsdl_la_LDFLAGS=$(GLOBUS_LDFLAGS)


jsdl_job.cpp: jsdl_soapStub.h jsdl_soapC.cpp

jsdl_soap.h: jsdl_complete.xsd jsdl_POSIXApplication.xsd jsdl_arc.xsd typemap.dat
	$(WSDL2H) -t typemap.dat -o jsdl_soap.h jsdl_POSIXApplication.xsd jsdl_arc.xsd jsdl_complete.xsd

jsdl_soapC.cpp jsdl_soapH.h jsdl_soapStub.h: jsdl_soap.h stlvector.h
	$(SOAPCPP2) -n -p jsdl_soap jsdl_soap.h

