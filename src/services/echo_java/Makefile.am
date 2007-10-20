pkglib_LTLIBRARIES = libarcservice_javawrapper.la

libarcservice_javawrapper_la_SOURCES = javawrapper.cpp javawrapper.h
libarcservice_javawrapper_la_CXXFLAGS = -fno-strict-aliasing $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(JDK_CFLAGS)
libarcservice_javawrapper_la_LDFLAGS = -no-undefined -avoid-version -module
libarcservice_javawrapper_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(JDK_LIBS)
