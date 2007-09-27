noinst_PROGRAMS = query

query_SOURCES=query.cpp
query_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
query_LDFLAGS=$(LIBXML2_LIBS) $(GLIBMM_LIBS) 
query_LDADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la
