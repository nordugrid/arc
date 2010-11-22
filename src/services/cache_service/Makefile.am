pkglib_LTLIBRARIES = libcacheservice.la

initddir=@initddir@
if SYSV_SCRIPTS_ENABLED
CACHE_SCRIPT = cache-service
else
CACHE_SCRIPT =
endif
initd_SCRIPTS = $(CACHE_SCRIPT)

libcacheservice_la_SOURCES = CacheService.h CacheService.cpp
libcacheservice_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libcacheservice_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
					$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
					$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
					$(top_srcdir)/src/hed/libs/ws/libarcws.la \
					$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
					../a-rex/grid-manager/libgridmanager.la \
					$(GLIBMM_LIBS)
libcacheservice_la_LDFLAGS = -no-defined -avoid-version -module
