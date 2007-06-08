JDK_INCLUDES = @JDK_INCLUDE@ -I$(top_builddir)

lib_LTLIBRARIES= libarcservice_javawrapper.la

libarcservice_javawrapper_ladir=$(includedir)
libarcservice_javawrapper_la_HEADERS=javawrapper.h
libarcservice_javawrapper_la_SOURCES=javawrapper.cpp
libarcservice_javawrapper_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/ $(JDK_INCLUDES) 
libarcservice_javawrapper_la_LDFLAGS=@JDK_LDFLAGS@
