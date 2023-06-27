SUBDIRS = schema

pkglib_LTLIBRARIES = libidentitymap.la

libidentitymap_la_SOURCES = IdentityMap.cpp SimpleMap.cpp \
                            IdentityMap.h   SimpleMap.h
libidentitymap_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
libidentitymap_la_LIBADD = \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) 
libidentitymap_la_LDFLAGS = -no-undefined -avoid-version -module
