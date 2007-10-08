pythondir= $(PY_SITE_PACKAGES)

python_DATA = arc.py
python_LTLIBRARIES = _arc.la

_arc_la_SOURCES = arc_wrap.cpp
_arc_la_CXXFLAGS = -DSWIG_COBJECT_TYPES $(PY_CFLAGS) -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
_arc_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la
_arc_la_LDFLAGS = -no-undefined -module -avoid-version

SWIG_OUTPUTS = arc_wrap.cpp arc.py

EXTRA_DIST = $(SWIG_OUTPUTS)

CLEANFILES = arc.pyc
MAINTAINERCLEANFILES = $(SWIG_OUTPUTS)

include ./$(DEPDIR)/arc_wrap.deps

arc_wrap.cpp arc.py: $(top_srcdir)/swig/*.i
	grep -h '^#' $(top_srcdir)/swig/*.i | \
	$(CXXCOMPILE) $(_arc_la_CXXFLAGS) -M -MT arc_wrap.cpp -MT arc.py -MP -MF "$(DEPDIR)/arc_wrap.deps" -x c++ -
	$(SWIG) -v -c++ -python -module arc -o arc_wrap.cpp \
		-I$(top_srcdir)/include $(top_srcdir)/swig/Arc.i
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
	rm arc_wrap.cpp.bak
