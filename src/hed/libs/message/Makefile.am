lib_LTLIBRARIES=libmessage.la
SUBDIRS=
noinst_PROGRAMS=
SOURCES = 

libmessage_ladir=$(includedir)
libmessage_la_HEADERS=SOAPMessage.h   PayloadRaw.h   PayloadSOAP.h   PayloadStream.h   MCC.h   Service.h   MessageAttributes.h   Message.h
libmessage_la_SOURCES=SOAPMessage.cpp PayloadRaw.cpp PayloadSOAP.cpp PayloadStream.cpp MCC.cpp Service.cpp MessageAttributes.cpp
libmessage_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libmessage_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la
#libmessage_la_LDFLAGS=$(LIBXML2_LIBS)

