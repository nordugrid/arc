lib_LTLIBRARIES = libarcclient.la
#noinst_PROGRAMS = test_broker
noinst_PROGRAMS = test_jobdescription

libarcclient_ladir = $(pkgincludedir)/client
libarcclient_la_HEADERS  = ACC.h Broker.h ExecutionTarget.h \
	Submitter.h TargetRetriever.h TargetGenerator.h \
	Job.h JobController.h JobSupervisor.h JobDescription.h \
	ClientInterface.h UserConfig.h
libarcclient_la_SOURCES  = ACC.cpp Broker.cpp ExecutionTarget.cpp \
	Submitter.cpp TargetRetriever.cpp TargetGenerator.cpp \
	Job.cpp JobController.cpp JobSupervisor.cpp JobDescription.cpp \
	ClientInterface.cpp UserConfig.cpp
libarcclient_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarcclient_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
test_jobdescription_SOURCES  = test_jobdescription.cpp
test_jobdescription_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_jobdescription_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_jobdescription_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la libarcclient.la

#test_broker_SOURCES  = test_broker.cpp
#test_broker_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#test_broker_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
#test_broker_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la
