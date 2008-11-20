pkglib_LTLIBRARIES = libpythonservice.la

libpythonservice_la_SOURCES  = pythonwrapper.cpp pythonwrapper.h
libpythonservice_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(PYTHON_CFLAGS)
libpythonservice_la_LDFLAGS  = -no-undefined -avoid-version -module
libpythonservice_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                               $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                               $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(LIBXML2_LIBS) $(PYTHON_LIBS) $(GLIBMM_LIBS)

