DIST_SUBDIRS = test
SUBDIRS = . $(TEST_DIR)

noinst_LTLIBRARIES = libdatastaging.la

libdatastaging_la_SOURCES = DTR.cpp DTR.h Scheduler.cpp Scheduler.h \
  Processor.cpp Processor.h DTRList.h DTRList.cpp \
  DataDelivery.cpp DataDelivery.h \
  DataDeliveryComm.cpp DataDeliveryComm.h \
  DTRStatus.h DTRStatus.cpp \
  TransferShares.h TransferShares.cpp

libdatastaging_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libdatastaging_la_LIBADD = $(top_srcdir)/src/hed/libs/data/libarcdata2.la

noinst_PROGRAMS = generator

generator_SOURCES = generator-main.cpp Generator.h Generator.cpp 
generator_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
generator_LDADD = libdatastaging.la

pkglibexec_PROGRAMS = DataStagingDelivery CacheManager

DataStagingDelivery_SOURCES = DataStagingDelivery.cpp
DataStagingDelivery_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
DataStagingDelivery_LDADD = \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)

CacheManager_SOURCES = CacheManager.cpp
CacheManager_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
CacheManager_LDADD = libdatastaging.la
