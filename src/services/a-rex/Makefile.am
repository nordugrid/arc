if JURA_CLIENT_ENABLED
JURA_CLIENT = jura
else
JURA_CLIENT =
endif

SUBDIRS = delegation grid-manager ldif infoproviders $(JURA_CLIENT) lrms schema internaljobplugin rte
DIST_SUBDIRS = delegation grid-manager ldif infoproviders jura lrms schema internaljobplugin rte

pkglib_LTLIBRARIES = libarex.la
noinst_PROGRAMS = test test_cache_check
if PYTHON_LRMS_ENABLED
    PYTHON_LRMS_PKGLIBEXEC_FILES = arc-sshfs-mount
else
    PYTHON_LRMS_PKGLIBEXEC_FILES =
endif

pkglibexec_SCRIPTS = $(PYTHON_LRMS_PKGLIBEXEC_FILES)

if SYSV_SCRIPTS_ENABLED
AREX_SCRIPT = arc-arex
else
AREX_SCRIPT =
endif
initd_SCRIPTS = $(AREX_SCRIPT)

if SYSTEMD_UNITS_ENABLED
AREX_UNIT = arc-arex.service
AREX_UNIT_WRAPPER = arc-arex-start
else
AREX_UNIT =
AREX_UNIT_WRAPPER =
endif
units_DATA = $(AREX_UNIT)
pkgdata_SCRIPTS = $(AREX_UNIT_WRAPPER) perferator

sbin_SCRIPTS = a-rex-backtrace-collect

man_MANS = a-rex-backtrace-collect.8

EXTRA_DIST = arc-sshfs-mount arc.zero.conf

GRIDMANAGER_LIBS = grid-manager/libgridmanager.la delegation/libdelegation.la

libarex_la_SOURCES  = arex.cpp job.cpp \
	create_activity.cpp get_activity_documents.cpp \
	get_activity_statuses.cpp terminate_activities.cpp \
	change_activity_status.cpp migrate_activity.cpp \
	get_factory_attributes_document.cpp update_credentials.cpp faults.cpp \
	get.cpp put.cpp PayloadFile.cpp FileChunks.cpp \
	information_collector.cpp cachecheck.cpp tools.cpp \
	arex.h job.h PayloadFile.h FileChunks.h tools.h
libarex_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(DBCXX_CPPFLAGS) $(AM_CXXFLAGS)
# Needs real cleaning in respect to dependencies
libarex_la_LIBADD = \
	ldif/libldif.la \
	$(GRIDMANAGER_LIBS) \
	$(top_builddir)/src/hed/libs/infosys/libarcinfosys.la \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la
libarex_la_LDFLAGS = -no-undefined -avoid-version -module $(DBCXX_LIBS)

test_SOURCES = test.cpp
test_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
test_LDADD = \
	$(top_builddir)/src/hed/libs/infosys/libarcinfosys.la \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

test_cache_check_SOURCES = test_cache_check.cpp
test_cache_check_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
test_cache_check_LDADD = \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(sysconfdir)"
	if test ! -e $(DESTDIR)$(sysconfdir)/arc.conf; then $(INSTALL_DATA) arc.zero.conf $(DESTDIR)$(sysconfdir)/arc.conf; fi

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/arc.conf

