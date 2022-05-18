noinst_LTLIBRARIES = libdelegation.la

if DBCXX_ENABLED
FILERECORDBDB_SOURCE = FileRecordBDB.cpp
FILERECORDBDB_HEADER = FileRecordBDB.h
else
FILERECORDBDB_SOURCE =
FILERECORDBDB_HEADER =
endif

libdelegation_la_SOURCES = \
	uid.cpp FileRecord.cpp $(FILERECORDBDB_SOURCE) FileRecordSQLite.cpp \
	DelegationStore.cpp DelegationStores.cpp \
	uid.h   FileRecord.h   $(FILERECORDBDB_HEADER) FileRecordSQLite.h   \
	DelegationStore.h   DelegationStores.h \
	../SQLhelpers.h
libdelegation_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(DBCXX_CPPFLAGS) $(SQLITE_CFLAGS) $(AM_CXXFLAGS)
libdelegation_la_LIBADD = $(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(DBCXX_LIBS) $(SQLITE_LIBS)

