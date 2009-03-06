pythondir= $(PYTHON_SITE_PACKAGES)

python_DATA = arc.py
python_LTLIBRARIES = _arc.la

_arc_la_SOURCES = arc_init.cpp arc_wrap.cpp
_arc_la_CXXFLAGS = -fno-strict-aliasing -DSWIG_COBJECT_TYPES $(PYTHON_CFLAGS) -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
_arc_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(PYTHON_LIBS)
_arc_la_LDFLAGS = -no-undefined -module -avoid-version

SWIG_OUTPUTS = arc_wrap.cpp arc.py

EXTRA_DIST = $(SWIG_OUTPUTS)

CLEANFILES = arc.pyc
MAINTAINERCLEANFILES = $(SWIG_OUTPUTS)

include ./$(DEPDIR)/arc_wrap.deps

arc_wrap.cpp arc.py: $(top_srcdir)/swig/*.i
	grep -h '^#' $(top_srcdir)/swig/*.i | \
	$(CXXCOMPILE) $(_arc_la_CXXFLAGS) -M -MT arc_wrap.cpp -MT arc.py -MP -MF "$(DEPDIR)/arc_wrap.deps" -x c++ -
	$(SWIG) -v -c++ -python -threads -module arc -o arc_wrap.cpp \
		-I$(top_srcdir)/include $(top_srcdir)/swig/Arc.i
