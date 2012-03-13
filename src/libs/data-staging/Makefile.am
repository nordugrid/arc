DIST_SUBDIRS = test
SUBDIRS = . $(TEST_DIR)

lib_LTLIBRARIES = libarcdatastaging.la

libarcdatastaging_ladir = $(pkgincludedir)/data-staging

libarcdatastaging_la_HEADERS = DataDelivery.h DataDeliveryComm.h \
  DataDeliveryLocalComm.h DataDeliveryRemoteComm.h DTR.h DTRList.h \
  DTRStatus.h Processor.h Scheduler.h TransferShares.h 

libarcdatastaging_la_SOURCES = DataDelivery.cpp DataDeliveryComm.cpp \
  DataDeliveryLocalComm.cpp DataDeliveryRemoteComm.cpp DTR.cpp DTRList.cpp \
  DTRStatus.cpp Processor.cpp Scheduler.cpp TransferShares.cpp

libarcdatastaging_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libarcdatastaging_la_LIBADD = \
  $(top_builddir)/src/hed/libs/common/libarccommon.la \
  $(top_builddir)/src/hed/libs/credential/libarccredential.la \
  $(top_builddir)/src/hed/libs/data/libarcdata2.la \
  $(top_builddir)/src/hed/libs/message/libarcmessage.la \
  $(top_builddir)/src/hed/libs/loader/libarcloader.la \
  $(top_builddir)/src/hed/libs/client/libarcclient.la \
  $(GLIBMM_LIBS)
libarcdatastaging_la_LDFLAGS = -version-info 1:0:0

noinst_PROGRAMS = generator

generator_SOURCES = generator-main.cpp Generator.h Generator.cpp 
generator_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
generator_LDADD = libarcdatastaging.la

pkglibexec_PROGRAMS = DataStagingDelivery

DataStagingDelivery_SOURCES = DataStagingDelivery.cpp
DataStagingDelivery_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
DataStagingDelivery_LDADD = \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)
