pkglib_LTLIBRARIES = libcacheservice.la

if SYSV_SCRIPTS_ENABLED
CACHE_SCRIPT = arc-cache-service
else
CACHE_SCRIPT =
endif
initd_SCRIPTS = $(CACHE_SCRIPT)

if SYSTEMD_UNITS_ENABLED
CACHE_UNIT = arc-cache-service.service
CACHE_UNIT_WRAPPER = arc-cache-service-start
else
CACHE_UNIT =
CACHE_UNIT_WRAPPER =
endif
units_DATA = $(CACHE_UNIT)
pkgdata_SCRIPTS = $(CACHE_UNIT_WRAPPER)

libcacheservice_la_SOURCES = CacheService.h CacheService.cpp \
                             CacheServiceGenerator.h CacheServiceGenerator.cpp
libcacheservice_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
libcacheservice_la_LIBADD = \
	../a-rex/grid-manager/libgridmanager.la \
	../a-rex/delegation/libdelegation.la \
	$(top_builddir)/src/hed/libs/infosys/libarcinfosys.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)
libcacheservice_la_LDFLAGS = -no-undefined -avoid-version -module
