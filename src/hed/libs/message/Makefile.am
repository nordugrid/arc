lib_LTLIBRARIES = libarcmessage.la

libarcmessage_ladir = $(pkgincludedir)/message
libarcmessage_la_HEADERS  = SOAPEnvelope.h   PayloadRaw.h   PayloadSOAP.h   PayloadStream.h   MCC_Status.h MCC.h     Service.h   MessageAttributes.h   Message.h   SOAPMessage.h   MessageAuth.h
libarcmessage_la_SOURCES  = SOAPEnvelope.cpp PayloadRaw.cpp PayloadSOAP.cpp PayloadStream.cpp MCC_Status.cpp MCC.cpp Service.cpp MessageAttributes.cpp Message.cpp SOAPMessage.cpp MessageAuth.cpp
libarcmessage_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcmessage_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(LIBXML2_LIBS)
