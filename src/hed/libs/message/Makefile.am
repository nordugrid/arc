lib_LTLIBRARIES=libmessage.la
SUBDIRS=
noinst_PROGRAMS=
SOURCES = 

libmessage_ladir=$(includedir)
libmessage_la_HEADERS=SOAPEnvelop.h   PayloadRaw.h   PayloadSOAP.h   PayloadStream.h   MCC_Status.h MCC.h   Service.h   MessageAttributes.h   Message.h SOAPMessage.h
libmessage_la_SOURCES=SOAPEnvelop.cpp PayloadRaw.cpp PayloadSOAP.cpp PayloadStream.cpp MCC_Status.cpp MCC.cpp Service.cpp MessageAttributes.cpp Message.cpp SOAPMessage.cpp
libmessage_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libmessage_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la
#libmessage_la_LDFLAGS=$(LIBXML2_LIBS)

