noinst_PROGRAMS = query

query_SOURCES = query.cpp
query_CXXFLAGS = -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
query_LDADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
