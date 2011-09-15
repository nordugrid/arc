SUBDIRS = schema

pkglib_LTLIBRARIES = libjavaservice.la

libjavaservice_la_SOURCES = javawrapper.cpp javawrapper.h
libjavaservice_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(JDK_CFLAGS) \
	-fno-strict-aliasing
libjavaservice_la_LDFLAGS = -no-undefined -avoid-version -module
libjavaservice_la_LIBADD = \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)
