noinst_PROGRAMS = submittest

lib_LTLIBRARIES = libarcclient.la

libarcclient_ladir = $(pkgincludedir)/client
libarcclient_la_HEADERS  = ACC.h Broker.h ExecutionTarget.h \
	Submitter.h TargetRetriever.h TargetGenerator.h \
	Job.h JobController.h JobSupervisor.h JobDescription.h \
	ClientInterface.h
libarcclient_la_SOURCES  = ACC.cpp Broker.cpp ExecutionTarget.cpp \
	Submitter.cpp TargetRetriever.cpp TargetGenerator.cpp \
	Job.cpp JobController.cpp JobSupervisor.cpp JobDescription.cpp \
	ClientInterface.cpp
libarcclient_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarcclient_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)

submittest_SOURCES  = submittest.cpp
submittest_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	-I$(top_srcdir)/include
submittest_LDADD    = libarcclient.la \
	../common/libarccommon.la ../loader/libarcloader.la
