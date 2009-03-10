lib_LTLIBRARIES = libarcmessage.la
noinst_PROGRAMS = test
noinst_LTLIBRARIES = libtestmcc.la

libarcmessage_ladir = $(pkgincludedir)/message
libarcmessage_la_HEADERS  = SOAPEnvelope.h   PayloadRaw.h   PayloadSOAP.h   PayloadStream.h   MCC_Status.h   MCC.h   Service.h   Plexer.h   MessageAttributes.h   Message.h   SOAPMessage.h   MessageAuth.h   SecAttr.h   MCCLoader.h
libarcmessage_la_SOURCES  = SOAPEnvelope.cpp PayloadRaw.cpp PayloadSOAP.cpp PayloadStream.cpp MCC_Status.cpp MCC.cpp Service.cpp Plexer.cpp MessageAttributes.cpp Message.cpp SOAPMessage.cpp MessageAuth.cpp SecAttr.cpp MCCLoader.cpp
libarcmessage_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcmessage_la_LIBADD   = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la

libtestmcc_la_SOURCES  = TestMCC.cpp TestMCC.h
libtestmcc_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libtestmcc_la_LIBADD   = $(GLIBMM_LIBS) ./libarcmessage.la $(top_srcdir)/src/hed/libs/common/libarccommon.la 

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDADD    = ./libarcmessage.la

