pkglib_LTLIBRARIES = libjavaservice.la

libjavaservice_la_SOURCES  = javawrapper.cpp javawrapper.h
libjavaservice_la_CXXFLAGS = -fno-strict-aliasing $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(JDK_CFLAGS)
libjavaservice_la_LDFLAGS  = -no-undefined -avoid-version -module
libjavaservice_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                             $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                             $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(LIBXML2_LIBS) $(JDK_LIBS)
