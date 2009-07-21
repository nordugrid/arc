noinst_LTLIBRARIES = libsrmclientsoap.la libsrmclient.la 

SRM1Client.h: srm1_soapH.h
SRM22Client.h: srm2_2_soapH.h


nodist_libsrmclientsoap_la_SOURCES = srm1_soapClientLib.cpp srm2_2_soapClientLib.cpp
libsrmclientsoap_la_CXXFLAGS = -fno-strict-aliasing $(GSOAP_CFLAGS) $(GLOBUS_IO_CFLAGS)
libsrmclientsoap_la_LIBADD = $(GSOAP_LIBS)

libsrmclient_la_SOURCES = SRMClient.h SRMClient.cpp SRM1Client.cpp SRM1Client.h SRM22Client.cpp SRM22Client.h SRMURL.cpp SRMURL.h
libsrmclient_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(GLOBUS_IO_CFLAGS) $(GSOAP_CFLAGS) $(CC_MISC_FLAGS) -I$(top_srcdir)/include
libsrmclient_la_LIBADD = libsrmclientsoap.la ../https/libhttpsclient.la

MOSTLYCLEANFILES = srm1_soap* srm2_2_soap* *.xml

EXTRA_DIST = srm.v1.wsdl srm.v2.2.wsdl typemat.dat

soaptargets = srm1_soap srm2_2_soap

# Exclicit rules
srm2_2_soap.h: srm.v2.2.wsdl typemat.dat
	$(WSDL2H) -t typemat.dat -s -o $@ $<

srm1_soap.h: srm.v1.wsdl typemat.dat
	$(WSDL2H) -t typemat.dat -s -o $@ $<

# Generic rules
${soaptargets:%=%ClientLib.cpp}: %ClientLib.cpp: %Client.cpp %C.cpp

${soaptargets:%=%ServerLib.cpp}: %ServerLib.cpp: %Server.cpp %C.cpp

${soaptargets:%=%C.cpp}: %C.cpp: %H.h

${soaptargets:%=%Client.cpp}: %Client.cpp: %H.h

${soaptargets:%=%Server.cpp}: %Server.cpp: %H.h

${soaptargets:%=%.nsmap}: %.nsmap: %H.h

${soaptargets:%=%H.h}: %H.h: %Stub.h

# The sed commands below is a workaround for gSOAP 2.7.13 (eg. Fedora 11) that makes
# soap_instantiate_* functions non-static. See:
# http://sourceforge.net/tracker/?func=detail&aid=2814405&group_id=52781&atid=468021

${soaptargets:%=%Stub.h}: %Stub.h: %.h
	$(SOAPCPP2) -n -p $* $*.h
	sed 's/^SOAP_FMAC3 \(.*\) soap_instantiate_/\1 soap_instantiate_/' < $*C.cpp > $*C.cpp_new && mv $*C.cpp_new $*C.cpp
	sed 's/^SOAP_FMAC3 \(.*\) soap_instantiate_/\1 soap_instantiate_/' < $*H.h > $*H.h_new && mv $*H.h_new $*H.h

BUILT_SOURCES = ${soaptargets:%=%H.h}
