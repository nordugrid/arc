lib_LTLIBRARIES = libarccompute.la
noinst_PROGRAMS = test_jobdescription

libarccompute_ladir = $(pkgincludedir)/client
libarccompute_la_HEADERS = Broker.h ExecutionTarget.h \
	Submitter.h SubmitterPlugin.h GLUE2Entity.h \
	JobState.h Job.h JobControllerPlugin.h JobSupervisor.h \
	JobDescription.h JobDescriptionParser.h Software.h \
	GLUE2.h EndpointQueryingStatus.h TestACCControl.h \
	EntityRetriever.h Endpoint.h \
	ComputingServiceRetriever.h
libarccompute_la_SOURCES = Broker.cpp ExecutionTarget.cpp \
	Submitter.cpp SubmitterPlugin.cpp \
	JobState.cpp Job.cpp JobControllerPlugin.cpp JobSupervisor.cpp \
	JobDescription.cpp JobDescriptionParser.cpp Software.cpp \
	GLUE2.cpp EndpointQueryingStatus.cpp TestACCControl.cpp \
	EntityRetriever.cpp Endpoint.cpp \
	ComputingServiceRetriever.cpp
libarccompute_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(OPENSSL_CFLAGS)
libarccompute_la_LIBADD = \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libarccompute_la_LDFLAGS = -version-info 2:0:0

test_jobdescription_SOURCES = test_jobdescription.cpp
test_jobdescription_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
test_jobdescription_LDADD = libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)

testjobdir = $(pkgdatadir)/test-jobs
testjob_DATA = test-job-1 test-job-2 test-job-3
EXTRA_DIST = $(testjob_DATA)

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)