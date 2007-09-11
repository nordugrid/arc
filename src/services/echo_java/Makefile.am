JDK_INCLUDES = @JDK_INCLUDE@ -I$(top_builddir)

lib_LTLIBRARIES= libarcservice_javawrapper.la

libarcservice_javawrapper_la_SOURCES=javawrapper.cpp javawrapper.h
libarcservice_javawrapper_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/ -I$(top_srcdir)/src/hed/libs/ $(JDK_INCLUDES) 
libarcservice_javawrapper_la_LDFLAGS=@JDK_LDFLAGS@
