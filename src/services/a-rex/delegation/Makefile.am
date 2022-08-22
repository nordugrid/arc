noinst_LTLIBRARIES = libdelegation.la

libdelegation_la_SOURCES = \
	uid.cpp FileRecord.cpp FileRecordSQLite.cpp \
	DelegationStore.cpp DelegationStores.cpp \
	uid.h   FileRecord.h   FileRecordSQLite.h   \
	DelegationStore.h   DelegationStores.h \
	../SQLhelpers.h
libdelegation_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(SQLITE_CFLAGS) $(AM_CXXFLAGS)
libdelegation_la_LIBADD = $(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(SQLITE_LIBS)

