noinst_LTLIBRARIES = libsrmclientsoap.la libsrmclient.la 

SRMClient.cpp: srm1_soap.nsmap srm2_2_soap.nsmap

srm1_soapClientLib.cpp: srm1_soap.nsmap
srm2_2_soapClientLib.cpp: srm2_2_soap.nsmap

srm2_2_soap.nsmap: srm.v2.2.wsdl typemat.dat
	$(WSDL2H) -t typemat.dat -s -o srm2_2_soap.h srm.v2.2.wsdl
	$(SOAPCPP2) -n -p srm2_2_soap srm2_2_soap.h

srm1_soap.nsmap: srm.v1.wsdl typemat.dat
	$(WSDL2H) -t typemat.dat -s -o srm1_soap.h srm.v1.wsdl
	$(SOAPCPP2) -n -p srm1_soap srm1_soap.h

nodist_libsrmclientsoap_la_SOURCES = srm1_soapClientLib.cpp srm2_2_soapClientLib.cpp
libsrmclientsoap_la_CXXFLAGS = -fno-strict-aliasing $(GSOAP_CFLAGS) $(GLOBUS_IO_CFLAGS)
libsrmclientsoap_la_LIBADD = $(GSOAP_LIBS)

libsrmclient_la_SOURCES = SRMClient.h SRMClient.cpp SRM1Client.cpp SRM1Client.h SRM22Client.cpp SRM22Client.h SRMURL.cpp SRMURL.h
libsrmclient_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(GLOBUS_IO_CFLAGS) $(GSOAP_CFLAGS) $(CC_MISC_FLAGS) -I$(top_srcdir)/include
libsrmclient_la_LIBADD = libsrmclientsoap.la ../https/libhttpsclient.la

MOSTLYCLEANFILES = srm1_soap* srm2_soap* *.xml srm2_2_soap* *.xml srm.so *.o

EXTRA_DIST = srm.v1.wsdl srm.v2.2.wsdl typemat.dat
