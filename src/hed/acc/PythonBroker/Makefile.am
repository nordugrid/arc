pkglib_LTLIBRARIES = libaccPythonBroker.la

libaccPythonBroker_la_SOURCES  = PythonBroker.cpp PythonBroker.h
libaccPythonBroker_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(PYTHON_CFLAGS)
libaccPythonBroker_la_LDFLAGS  = -no-undefined -avoid-version -module
libaccPythonBroker_la_LIBADD   = \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(PYTHON_LIBS)

doc_DATA = SampleBroker.py
EXTRA_DIST = $(doc_DATA)
