pkglib_LTLIBRARIES = libidentitymap.la

libidentitymap_la_SOURCES = IdentityMap.cpp simplemap.cpp \
                            IdentityMap.h   simplemap.h
libidentitymap_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libidentitymap_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(GLIBMM_LIBS) $(LIBXML2_LIBS) 
libidentitymap_la_LDFLAGS  = -no-undefined -avoid-version -module
