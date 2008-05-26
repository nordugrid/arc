noinst_PROGRAMS = submittest

lib_LTLIBRARIES = libarclibclient.la

libarclibclient_ladir = $(pkgincludedir)/client
libarclibclient_la_HEADERS  = ACC.h Broker.h ExecutionTarget.h \
	                      Submitter.h TargetRetriever.h TargetGenerator.h \
			      Job.h JobController.h JobSupervisor.h
libarclibclient_la_SOURCES  = ACC.cpp Broker.cpp ExecutionTarget.cpp \
	                      Submitter.cpp TargetRetriever.cpp TargetGenerator.cpp \
                              Job.cpp JobController.cpp JobSupervisor.cpp
libarclibclient_la_CXXFLAGS = -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarclibclient_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                              $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
		              $(top_srcdir)/src/hed/libs/misc/libarcclient.la \
                              $(LIBXML2_LIBS) $(GLIBMM_LIBS)

submittest_SOURCES  = submittest.cpp
submittest_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
submittest_LDADD    = libarclibclient.la ../common/libarccommon.la ../misc/libarcclient.la ../loader/libarcloader.la
