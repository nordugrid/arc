bin_PROGRAMS = arccp arcls arcrm

man_MANS = arccp.1 arcls.1 arcrm.1

arccp_SOURCES  = arccp.cpp
arccp_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arccp_LDADD    = ../../hed/libs/data/libarcdata2.la

arcls_SOURCES  = arcls.cpp
arcls_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcls_LDADD    = ../../hed/libs/data/libarcdata2.la

arcrm_SOURCES  = arcrm.cpp
arcrm_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcrm_LDADD    = ../../hed/libs/data/libarcdata2.la

EXTRA_DIST = arccli.cpp
