bin_PROGRAMS = arccp arcls arcrm

man_MANS = arccp.1 arcls.1 arcrm.1

CLILIBS = $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la

arccp_SOURCES  = arccp.cpp
arccp_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arccp_LDADD    = $(CLILIBS)

arcls_SOURCES  = arcls.cpp
arcls_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcls_LDADD    = $(CLILIBS)

arcrm_SOURCES  = arcrm.cpp
arcrm_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcrm_LDADD    = $(CLILIBS)
