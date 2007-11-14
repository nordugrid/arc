noinst_LTLIBRARIES = libjsdl.la

libjsdl_la_SOURCES=jsdl_job.cpp jsdl_job.h
# nodist_libjsdl_la_SOURCES = jsdl_soapClient.cpp jsdl_soapC.cpp jsdl_soapStub.h stlvector.h
libjsdl_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_RSL_CFLAGS) -I$(top_srcdir)/include
libjsdl_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la 
#../../files/libfiles.la ../../misc/libmisc.la ../../url/liburl.la
libjsdl_la_LDFLAGS=

#libnamespace_la_SOURCES=namespace.cpp
#libnamespace_la_CXXFLAGS=$(GSOAP_CFLAGS)
#libnamespace_la_LDFLAGS=

#BUILT_SOURCES = $(SOAPCPP2_OUTPUT) jsdl_soap.h

#SOAPCPP2_OUTPUT = jsdl_soapStub.h jsdl_soapH.h jsdl_soapC.cpp jsdl_soapClient.cpp jsdl_soapClientLib.cpp jsdl_soapServer.cpp jsdl_soapServerLib.cpp jsdl_soap.nsmap

#jsdl_soap.h: jsdl_complete.xsd jsdl_POSIXApplication.xsd jsdl_arc.xsd typemap.dat
#	$(WSDL2H) -t typemap.dat -o jsdl_soap.h jsdl_POSIXApplication.xsd jsdl_arc.xsd jsdl_complete.xsd

#$(SOAPCPP2_OUTPUT): jsdl_soap.h
#	$(SOAPCPP2) -n -p jsdl_soap $<

#CLEANFILES = $(SOAPCPP2_OUTPUT) jsdl_soap.h

EXTRA_DIST = *.xsd 
