JDK_INCLUDES = @JDK_INCLUDE@

lib_LTLIBRARIES= libarcservice_javawrapper.la

libarcservice_javawrapper_la_SOURCES=javawrapper.cpp javawrapper.h
libarcservice_javawrapper_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(JDK_INCLUDES) 
libarcservice_javawrapper_la_LDFLAGS=@JDK_LDFLAGS@
