pkglib_LTLIBRARIES = libacctargetretriever.la

libacctargetretriever_la_SOURCES  = TargetRetrieverARC0.cpp TargetRetrieverARC0.h
libacctargetretriever_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libacctargetretriever_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la
libacctargetretriever_la_LDFLAGS  = -no-undefined -avoid-version -module
