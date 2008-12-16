lib_LTLIBRARIES = libarcclient.la
noinst_PROGRAMS = test_jobdescription

if XMLSEC_ENABLED
LIBS_WITH_XMLSEC = $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
CFLAGS_WITH_XMLSEC = $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS)  
HEADER_WITH_XMLSEC = ClientSAML2SSO.h
SOURCE_WITH_XMLSEC = ClientSAML2SSO.cpp
else
LIBS_WITH_XMLSEC =
CFLAGS_WITH_XMLSEC =
HEADER_WITH_XMLSEC =
SOURCE_WITH_XMLSEC =
endif

libarcclient_ladir = $(pkgincludedir)/client
libarcclient_la_HEADERS  = ACC.h ACCLoader.h Broker.h ExecutionTarget.h \
	Submitter.h TargetRetriever.h TargetGenerator.h \
	Job.h JobController.h JobSupervisor.h JobDescription.h \
	ClientInterface.h $(HEADER_WITH_XMLSEC) UserConfig.h
libarcclient_la_SOURCES  = ACC.cpp ACCLoader.cpp Broker.cpp ExecutionTarget.cpp \
	Submitter.cpp TargetRetriever.cpp TargetGenerator.cpp \
	Job.cpp JobController.cpp JobSupervisor.cpp JobDescription.cpp \
	ClientInterface.cpp $(SOURCE_WITH_XMLSEC) UserConfig.cpp
libarcclient_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(CFLAGS_WITH_XMLSEC) $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarcclient_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(LIBS_WITH_XMLSEC) $(LIBXML2_LIBS) $(GLIBMM_LIBS)

test_jobdescription_SOURCES  = test_jobdescription.cpp
test_jobdescription_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_jobdescription_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_jobdescription_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la libarcclient.la

