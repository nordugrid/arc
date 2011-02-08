#DIST_SUBDIRS = test
#SUBDIRS = . $(TEST_DIR)

noinst_LTLIBRARIES = libdatastaging.la

libdatastaging_la_SOURCES=DTR.cpp DTR.h Scheduler.cpp Scheduler.h \
  Processor.cpp Processor.h DTRList.h DTRList.cpp \
  DataDelivery.cpp DataDelivery.h \
  DataDeliveryComm.cpp DataDeliveryComm.h \
  DTRStatus.h DTRStatus.cpp \
  TransferShares.h TransferShares.cpp

libdatastaging_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libdatastaging_la_LIBADD=$(top_srcdir)/src/hed/libs/data/libarcdata2.la

noinst_PROGRAMS = generator

generator_SOURCES = generator-main.cpp Generator.h Generator.cpp 
generator_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
generator_LDADD = libdatastaging.la

pkglibexec_PROGRAMS = DataStagingDelivery CacheManager

DataStagingDelivery_SOURCES = DataStagingDelivery.cpp
DataStagingDelivery_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
DataStagingDelivery_LDADD = $(top_srcdir)/src/hed/libs/data/libarcdata2.la

CacheManager_SOURCES = CacheManager.cpp
CacheManager_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
CacheManager_LDADD = libdatastaging.la
