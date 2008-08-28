lib_LTLIBRARIES = libarcclient.la

libarcclient_ladir = $(pkgincludedir)/client
libarcclient_la_HEADERS  = ACC.h DefaultBroker.h ExecutionTarget.h \
	Submitter.h TargetRetriever.h TargetGenerator.h \
	Job.h JobController.h JobSupervisor.h JobDescription.h \
	ClientInterface.h UserConfig.h  \
        RandomBroker.h SmartBroker.h	
libarcclient_la_SOURCES  = ACC.cpp DefaultBroker.cpp ExecutionTarget.cpp \
	Submitter.cpp TargetRetriever.cpp TargetGenerator.cpp \
	Job.cpp JobController.cpp JobSupervisor.cpp JobDescription.cpp \
	ClientInterface.cpp UserConfig.cpp  \
        RandomBroker.cpp SmartBroker.cpp
libarcclient_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarcclient_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
