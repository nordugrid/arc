bin_PROGRAMS = arcecho
SOURCES = echo_client.cpp echo_client.h
doc_DATA = echo_client.xml

arcecho_SOURCES  = $(SOURCES) arcecho.cpp
arcecho_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
arcecho_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                  $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(GLIBMM_LIBS) $(LIBXML2_LIBS)

EXTRA_DIST = $(doc_DATA)

man_MANS = arcecho.1
