pkglib_LTLIBRARIES = libdmcrls.la

libdmcrls_la_SOURCES  = DMCRLS.cpp DataPointRLS.cpp RLS.cpp \
                        DMCRLS.h   DataPointRLS.h   RLS.h
libdmcrls_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_RLS_CLIENT_CFLAGS) -I$(top_srcdir)/include
libdmcrls_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GLOBUS_RLS_CLIENT_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libdmcrls_la_LDFLAGS  = -no-undefined -avoid-version -module
