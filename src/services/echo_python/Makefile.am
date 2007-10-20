pkglib_LTLIBRARIES = libarcservice_pythonwrapper.la

libarcservice_pythonwrapper_la_SOURCES = pythonwrapper.cpp pythonwrapper.h
libarcservice_pythonwrapper_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(PYTHON_CFLAGS)
libarcservice_pythonwrapper_la_LDFLAGS = -no-undefined -avoid-version -module
libarcservice_pythonwrapper_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(PYTHON_LIBS)
