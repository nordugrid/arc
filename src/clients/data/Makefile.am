bin_PROGRAMS = arccp arcls arcrm

arccp_SOURCES  = arccp.cpp
arccp_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arccp_LDADD    = ../../hed/libs/data/libarcdata2.la \
                 ../../hed/libs/misc/libarcclient.la \
                 ../../hed/libs/loader/libarcloader.la \
                 ../../hed/libs/message/libarcmessage.la

arcls_SOURCES  = arcls.cpp
arcls_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcls_LDADD    = ../../hed/libs/data/libarcdata2.la \
                 ../../hed/libs/misc/libarcclient.la \
                 ../../hed/libs/loader/libarcloader.la \
                 ../../hed/libs/message/libarcmessage.la

arcrm_SOURCES  = arcrm.cpp
arcrm_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcrm_LDADD    = ../../hed/libs/data/libarcdata2.la \
                 ../../hed/libs/misc/libarcclient.la \
                 ../../hed/libs/loader/libarcloader.la \
                 ../../hed/libs/message/libarcmessage.la

EXTRA_DIST = arccli.cpp
