bin_PROGRAMS = arccp arcls arcrm

man_MANS = arccp.1 arcls.1 arcrm.1

CLILIBS = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la

arccp_SOURCES  = arccp.cpp
arccp_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arccp_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcls_SOURCES  = arcls.cpp
arcls_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcls_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcrm_SOURCES  = arcrm.cpp
arcrm_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcrm_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)
