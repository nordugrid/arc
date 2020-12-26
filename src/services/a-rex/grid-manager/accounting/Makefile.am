noinst_LTLIBRARIES = libaccounting.la

libaccounting_la_SOURCES = \
	AccountingDBSQLite.cpp AAR.cpp AccountingDBAsync.cpp \
	AccountingDBSQLite.h AccountingDB.h AAR.h AccountingDBAsync.h \
	../../SQLhelpers.h
libaccounting_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(SQLITE_CFLAGS) $(AM_CXXFLAGS)
libaccounting_la_LIBADD = \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(SQLITE_LIBS)

noinst_PROGRAMS = test_adb

test_adb_SOURCES = test_adb.cpp
test_adb_CXXFLAGS = -I$(top_srcdir)/include \
    $(GLIBMM_CFLAGS) $(SQLITE_CFLAGS) $(AM_CXXFLAGS)
test_adb_LDADD = libaccounting.la 

arcsqlschemadir = $(pkgdatadir)/sql-schema
arcsqlschema_DATA = arex_accounting_db_schema_v1.sql
EXTRA_DIST = $(arcsqlschema_DATA)

