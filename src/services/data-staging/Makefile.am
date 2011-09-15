pkglib_LTLIBRARIES = libdatadeliveryservice.la

if SYSV_SCRIPTS_ENABLED
DATA_DELIVERY_SCRIPT = arc-datadelivery-service
else
DATA_DELIVERY_SCRIPT =
endif
initd_SCRIPTS = $(DATA_DELIVERY_SCRIPT)

libdatadeliveryservice_la_SOURCES = DataDeliveryService.h DataDeliveryService.cpp
libdatadeliveryservice_la_CXXFLAGS = -I$(top_srcdir)/include \
  $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libdatadeliveryservice_la_LIBADD = \
  $(top_builddir)/src/libs/data-staging/libdatastaging.la \
  $(top_builddir)/src/hed/libs/ws/libarcws.la \
  $(top_builddir)/src/hed/libs/message/libarcmessage.la \
  $(top_builddir)/src/hed/libs/loader/libarcloader.la \
  $(top_builddir)/src/hed/libs/common/libarccommon.la \
  $(GLIBMM_LIBS)
libdatadeliveryservice_la_LDFLAGS = -no-undefined -avoid-version -module
