bin_PROGRAMS = apecho
SOURCES = echo_client.cpp echo_client.h

apecho_SOURCES  = $(SOURCES) apecho.cpp
apecho_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
apecho_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                  $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(GLIBMM_LIBS) $(LIBXML2_LIBS)
