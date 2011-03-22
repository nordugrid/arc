noinst_PROGRAMS = arcpolicy
SOURCES = arcpolicy.cpp

arcpolicy_SOURCES = $(SOURCES)
arcpolicy_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcpolicy_LDADD = \
	$(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS)
