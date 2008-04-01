noinst_PROGRAMS = test

lib_LTLIBRARIES = libarclibclient.la

libarclibclient_ladir = $(pkgincludedir)/client
libarclibclient_la_HEADERS  = ACC.h Broker.h ExecutionTarget.h \
	                      Submitter.h TargetRetriever.h TargetGenerator.h \
			      Endpoint.h Service.h ComputingEndpoint.h Share.h \
		              ComputingShare.h Resource.h ComputingResource.h \
			      Activity.h ComputingActivity.h ComputingService.h	
libarclibclient_la_SOURCES  = ACC.cpp Broker.cpp ExecutionTarget.cpp \
	                      Submitter.cpp TargetRetriever.cpp TargetGenerator.cpp
libarclibclient_la_CXXFLAGS = -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarclibclient_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(LIBXML2_LIBS) $(GLIBMM_LIBS)

test_SOURCES  = test.C
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDADD    = libarclibclient.la ../common/libarccommon.la ../misc/libarcclient.la ../loader/libarcloader.la

