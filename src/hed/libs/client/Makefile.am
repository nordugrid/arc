lib_LTLIBRARIES = libarcclient.la
noinst_PROGRAMS = test_jobdescription

if XMLSEC_ENABLED
ARCLIBS_WITH_XMLSEC = $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la
LIBS_WITH_XMLSEC = $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
CFLAGS_WITH_XMLSEC = $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS)
HEADER_WITH_XMLSEC = ClientSAML2SSO.h
SOURCE_WITH_XMLSEC = ClientSAML2SSO.cpp
else
ARCLIBS_WITH_XMLSEC =
LIBS_WITH_XMLSEC =
CFLAGS_WITH_XMLSEC =
HEADER_WITH_XMLSEC =
SOURCE_WITH_XMLSEC =
endif

libarcclient_ladir = $(pkgincludedir)/client
libarcclient_la_HEADERS = Broker.h ExecutionTarget.h \
	Submitter.h TargetRetriever.h TargetGenerator.h \
	JobState.h Job.h JobController.h JobSupervisor.h \
	JobDescription.h JobDescriptionParser.h Software.h \
	GLUE2.h \
	ClientInterface.h \
	ClientX509Delegation.h $(HEADER_WITH_XMLSEC)
libarcclient_la_SOURCES = Broker.cpp ExecutionTarget.cpp \
	Submitter.cpp TargetRetriever.cpp TargetGenerator.cpp \
	JobState.cpp Job.cpp JobController.cpp JobSupervisor.cpp \
	JobDescription.cpp JobDescriptionParser.cpp Software.cpp \
	GLUE2.cpp \
	ClientInterface.cpp \
	ClientX509Delegation.cpp $(SOURCE_WITH_XMLSEC)
libarcclient_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(CFLAGS_WITH_XMLSEC) $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarcclient_la_LIBADD = \
	$(ARCLIBS_WITH_XMLSEC) \
	$(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBS_WITH_XMLSEC) $(LIBXML2_LIBS) $(GLIBMM_LIBS)
libarcclient_la_LDFLAGS = -version-info 1:0:0

test_jobdescription_SOURCES = test_jobdescription.cpp
test_jobdescription_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
test_jobdescription_LDADD = libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
