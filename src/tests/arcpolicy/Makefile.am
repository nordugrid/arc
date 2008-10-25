noinst_PROGRAMS = arcpolicy
SOURCES = arcpolicy.cpp

arcpolicy_SOURCES  = $(SOURCES)
arcpolicy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcpolicy_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS)
arcpolicy_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                     $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                     $(top_srcdir)/src/hed/libs/security/libarcsecurity.la

