lib_LTLIBRARIES = libarcmessage.la
noinst_PROGRAMS = test

libarcmessage_ladir = $(pkgincludedir)/message
libarcmessage_la_HEADERS  = SOAPEnvelope.h   PayloadRaw.h   PayloadSOAP.h   PayloadStream.h   MCC_Status.h MCC.h     Service.h   MessageAttributes.h   Message.h   SOAPMessage.h   MessageAuth.h   SecAttr.h
libarcmessage_la_SOURCES  = SOAPEnvelope.cpp PayloadRaw.cpp PayloadSOAP.cpp PayloadStream.cpp MCC_Status.cpp MCC.cpp Service.cpp MessageAttributes.cpp Message.cpp SOAPMessage.cpp MessageAuth.cpp SecAttr.cpp
libarcmessage_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcmessage_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(GLIBMM_LIBS) $(LIBXML2_LIBS)

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDADD    = ./libarcmessage.la

