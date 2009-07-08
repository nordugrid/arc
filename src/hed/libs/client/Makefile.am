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

#some source for confusa, probably it should be in some independent directory
if OAUTH_LIBRARY_ENABLED
CXX_FLAGS_WITH_OAUTH = $(OAUTH_CFLAGS)
LIBS_WITH_OAUTH = $(OAUTH_LIBS)
HEADERS_WITH_OAUTH = confusa/OAuthConsumer.h
SOURCES_WITH_OAUTH = confusa/OAuthConsumer.cpp
else
CXX_FLAGS_WITH_OAUTH =
LIBS_WITH_OAUTH =
HEADERS_WITH_OAUTH =
SOURCES_WITH_OAUTH =
endif

HEADER_CONFUSA = confusa/ConfusaCertHandler.h confusa/SAML2LoginClient.h confusa/ConfusaParserUtils.h confusa/idp/HakaClient.h confusa/idp/OpenIdpClient.h $(HEADERS_WITH_OAUTH)
SOURCE_CONFUSA =  confusa/ConfusaCertHandler.cpp confusa/SAML2LoginClient.cpp confusa/ConfusaParserUtils.cpp confusa/idp/HakaClient.cpp confusa/idp/OpenIdpClient.cpp $(SOURCES_WITH_OAUTH)
CXX_FLAGS_CONFUSA = $(OPENSSL_CFLAGS) $(CXX_FLAGS_WITH_OAUTH)
LIBS_CONFUSA = $(OPENSSL_LIBS) $(LIBS_WITH_OAUTH)


libarcclient_ladir = $(pkgincludedir)/client
libarcclient_la_HEADERS  = ACC.h ACCLoader.h Broker.h ExecutionTarget.h \
  Submitter.h TargetRetriever.h TargetGenerator.h \
  JobState.h Job.h JobController.h JobSupervisor.h JobDescription.h JobDescriptionParser.h \
  ARCJSDLParser.h JDLParser.h XRSLParser.h SoftwareVersion.h \
  ClientInterface.h $(HEADER_WITH_XMLSEC) ClientX509Delegation.h UserConfig.h Sandbox.h RSLParser.h \
  $(HEADER_CONFUSA)
libarcclient_la_SOURCES  = ACC.cpp ACCLoader.cpp Broker.cpp ExecutionTarget.cpp \
  Submitter.cpp TargetRetriever.cpp TargetGenerator.cpp \
  Job.cpp JobController.cpp JobSupervisor.cpp JobDescription.cpp JobDescriptionParser.cpp \
  ARCJSDLParser.cpp JDLParser.cpp XRSLParser.cpp SoftwareVersion.cpp \
  ClientInterface.cpp $(SOURCE_WITH_XMLSEC) ClientX509Delegation.cpp UserConfig.cpp \
  Sandbox.cpp RSLParser.cpp $(SOURCE_CONFUSA) JobState.cpp
libarcclient_la_CXXFLAGS = -I$(top_srcdir)/include \
  $(CFLAGS_WITH_XMLSEC) $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(CXX_FLAGS_CONFUSA)
libarcclient_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
  $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
  $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
  $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
  $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
  $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
  $(top_srcdir)/src/hed/libs/ws-addressing/libwsaddressing.la \
  $(LIBS_WITH_XMLSEC) $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(LIBS_CONFUSA)

test_jobdescription_SOURCES  = test_jobdescription.cpp
test_jobdescription_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_jobdescription_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_jobdescription_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la libarcclient.la \
  $(top_srcdir)/src/hed/libs/ws-addressing/libwsaddressing.la

DIST_SUBDIRS = $(TEST_DIR)
SUBDIRS = $(TEST_DIR)
