pkglib_LTLIBRARIES = libarcservice_pythonwrapper.la

libarcservice_pythonwrapper_la_SOURCES=pythonwrapper.cpp pythonwrapper.h
libarcservice_pythonwrapper_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(PY_CFLAGS)
libarcservice_pythonwrapper_la_LDFLAGS=$(PY_EXTRA_LIBS) -no-undefined -avoid-version -module
