INCLUDES = -I$(top_srcdir) \
	-I$(top_builddir)

pythondir= $(PY_SITE_PACKAGES)
python_DATA = arc.py
python_LTLIBRARIES = _arc.la

_arc_la_SOURCES = arc_wrap.cpp
_arc_la_CPPFLAGS = -DSWIG_COBJECT_TYPES $(PY_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
_arc_la_LIBADD = $(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la
#if DARWIN
#_arc_la_LDFLAGS = -no-undefined -module -avoid-version -Wl,-F. -Wl,-F. -bundle -framework Python 
#else
#if MINGW
#_arc_la_LDFLAGS = -no-undefined -module -avoid-version -Wl,--add-stdcall-alias
#else
_arc_la_LDFLAGS = -no-undefined -module -avoid-version
#endif
#endif
SWIG_I_FILES=$(shell find $(top_srcdir)/swig/ -name '*.[ih]')
arc_wrap.cpp arc.py: $(SWIG_I_FILES)
	$(SWIG) -v -c++ -python -module arc -o arc_wrap.cpp \
		-D'__attribute__(x)=' $(top_srcdir)/swig/Arc.i
	cp arc_wrap.cpp arc_wrap.cpp.bak
	sed -e 's/^  int res = 0;$$//' \
			< arc_wrap.cpp.bak > arc_wrap.cpp
	cp arc_wrap.cpp arc_wrap.cpp.bak
	sed -e 's/\/\*  if (val) \*val = PyObject_IsTrue(obj); return 1; \*\//int res = 0;/' \
			< arc_wrap.cpp.bak > arc_wrap.cpp
	cp arc_wrap.cpp arc_wrap.cpp.bak
	grep -q "int res =" arc_wrap.cpp || \
		sed -e 's/if (obj == Py_True) /int res = 0; if (obj == Py_True) /' \
			< arc_wrap.cpp.bak > arc_wrap.cpp

SWIG_OUTPUTS = arc_wrap.cpp arc.py

clean-local:
	-rm arc_wrap.cpp.bak

# In distribution, swig generated files are present.
EXTRA_DIST = $(SWIG_OUTPUTS)

# Maintainer clean the swig generated files, because he/she maintains them.
MAINTAINERCLEANFILES = $(SWIG_OUTPUTS)
