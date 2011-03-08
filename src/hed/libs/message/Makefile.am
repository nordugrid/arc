lib_LTLIBRARIES = libarcmessage.la
noinst_PROGRAMS = testmcc
noinst_LTLIBRARIES = libtestmcc.la

libarcmessage_ladir = $(pkgincludedir)/message
libarcmessage_la_HEADERS  = SOAPEnvelope.h   PayloadRaw.h   PayloadSOAP.h   PayloadStream.h   MCC_Status.h   MCC.h   Service.h   Plexer.h   MessageAttributes.h   Message.h   SOAPMessage.h   MessageAuth.h   SecAttr.h   MCCLoader.h   SecHandler.h
libarcmessage_la_SOURCES  = SOAPEnvelope.cpp PayloadRaw.cpp PayloadSOAP.cpp PayloadStream.cpp MCC_Status.cpp MCC.cpp Service.cpp Plexer.cpp MessageAttributes.cpp Message.cpp SOAPMessage.cpp MessageAuth.cpp SecAttr.cpp MCCLoader.cpp SecHandler.cpp
libarcmessage_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcmessage_la_LIBADD   = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la
libarcmessage_la_LDFLAGS  = -version-info 1:0:0

libtestmcc_la_SOURCES  = TestMCC.cpp TestMCC.h
libtestmcc_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libtestmcc_la_LIBADD   = $(GLIBMM_LIBS) ./libarcmessage.la $(top_srcdir)/src/hed/libs/common/libarccommon.la 

testmcc_SOURCES  = testmcc.cpp
testmcc_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
testmcc_LDADD    = ./libarcmessage.la $(top_srcdir)/src/hed/libs/common/libarccommon.la

