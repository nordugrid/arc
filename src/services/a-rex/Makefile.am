SUBDIRS = grid-manager ldif infoproviders lrms logger urlogger schema

pkglib_LTLIBRARIES = libarex.la
noinst_PROGRAMS = test test_cache_check

if SYSV_SCRIPTS_ENABLED
AREX_SCRIPT = a-rex
else
AREX_SCRIPT =
endif
initd_SCRIPTS = $(AREX_SCRIPT)

exampledir = $(pkgdatadir)/examples/a-rex
example_DATA = arex.xml.example arex_secure.xml.example \
	arex_minimalistic.xml.example arex_xmlonly.xml.example \
	arex_showcase.xml.example arex_janitor.xml.example

GRIDMANAGER_LIBS = grid-manager/libgridmanager.la

libarex_la_SOURCES  = arex.cpp job.cpp \
	create_activity.cpp get_activity_documents.cpp \
	get_activity_statuses.cpp terminate_activities.cpp \
	change_activity_status.cpp migrate_activity.cpp \
	get_factory_attributes_document.cpp update_credentials.cpp faults.cpp \
	get.cpp put.cpp PayloadFile.cpp FileChunks.cpp \
	information_collector.cpp gacl.cpp cachecheck.cpp tools.cpp \
	arex.h job.h PayloadFile.h FileChunks.h gacl.h tools.h
libarex_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
# Needs real cleaning in respect to dependencies
libarex_la_LIBADD = \
	ldif/libldif.la \
	$(GRIDMANAGER_LIBS) \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la
libarex_la_LDFLAGS = -no-undefined -avoid-version -module

test_SOURCES = test.cpp
test_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
test_LDADD = \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

test_cache_check_SOURCES = test_cache_check.cpp
test_cache_check_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
test_cache_check_LDADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

EXTRA_DIST = $(example_DATA)
