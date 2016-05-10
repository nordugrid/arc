if SQLITE_ENABLED
FRSQLITE_SOURCES = FileRecordSQLite.cpp FileRecordSQLite.h
noinst_PROGRAMS = sqlite-test
else
FRSQLITE_SOURCES =
endif

noinst_LTLIBRARIES = libdelegation.la

libdelegation_la_SOURCES = \
	uid.cpp FileRecord.cpp FileRecordBDB.cpp DelegationStore.cpp DelegationStores.cpp \
	uid.h   FileRecord.h   FileRecordBDB.h   DelegationStore.h   DelegationStores.h \
	$(FRSQLITE_SOURCES)
libdelegation_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(DBCXX_CPPFLAGS) $(SQLITE_CFLAGS) $(AM_CXXFLAGS)
libdelegation_la_LIBADD = $(top_builddir)/src/hed/libs/common/libarccommon.la $(DBCXX_LIBS) $(SQLITE_LIBS)

if SQLITE_ENABLED
sqlite_test_SOURCES = sqlite_test.cpp
sqlite_test_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(DBCXX_CPPFLAGS) $(AM_CXXFLAGS)
sqlite_test_LDADD=-lsqlite3
endif
