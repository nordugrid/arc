pythondir = $(PYTHON_SITE_PACKAGES)

python_DATA = arc.py
python_LTLIBRARIES = _arc.la

if WIN32
AM_CPPFLAGS = -DWIN32 -DWINNT
endif

SWIG_INPUTS = $(top_srcdir)/swig/*.i

if PYDOXYGEN
PYDOXFLAGS = -DPYDOXYGEN
SWIG_INPUTS += pydoxygen.i
endif

_arc_la_SOURCES = arc_init.cpp arc_wrap.cpp
_arc_la_CXXFLAGS = -fno-strict-aliasing -DSWIG_COBJECT_TYPES $(PYTHON_CFLAGS) -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
_arc_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/credential/libarccredential.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(PYTHON_LIBS)
_arc_la_LDFLAGS = -no-undefined -module -avoid-version

SWIG_OUTPUTS = arc_wrap.cpp arc.py

EXTRA_DIST = $(SWIG_OUTPUTS) Doxyfile.api doxy2swig.py pydoxygen.i

CLEANFILES = arc.pyc api

MAINTAINERCLEANFILES = $(SWIG_OUTPUTS) pydoxygen.i

@AMDEP_TRUE@include ./$(DEPDIR)/arc_wrap.deps

arc_wrap.cpp arc.py: $(SWIG_INPUTS)
	mkdir -p $(DEPDIR)
	grep -h '^#' $(SWIG_INPUTS) | \
	$(CXXCOMPILE) $(_arc_la_CXXFLAGS) -M -MT arc_wrap.cpp -MT arc.py -MP -MF "$(DEPDIR)/arc_wrap.deps" -x c++ -
	$(SWIG) -v -c++ -python -threads -module arc -o arc_wrap.cpp \
		-I/usr/include -I$(top_srcdir)/include \
		$(PYDOXFLAGS) $(AM_CPPFLAGS) $(OPENSSL_CFLAGS) $(top_srcdir)/swig/Arc.i

pydoxygen.i: Doxyfile.api
	doxygen Doxyfile.api
	python doxy2swig.py api/xml/index.xml $@
	rm -rf api
