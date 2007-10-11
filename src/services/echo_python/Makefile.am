pkglib_LTLIBRARIES = libarcservice_pythonwrapper.la

libarcservice_pythonwrapper_la_SOURCES=pythonwrapper.cpp pythonwrapper.h
libarcservice_pythonwrapper_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(PYTHON_CFLAGS)
libarcservice_pythonwrapper_la_LDFLAGS=$(PYTHON_LIBS) -no-undefined -avoid-version -module
