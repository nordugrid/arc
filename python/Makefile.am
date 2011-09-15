pythondir = $(PYTHON_SITE_ARCH)

if PYTHON_ENABLED
python_DATA = arc.py
python_LTLIBRARIES = _arc.la
endif

altpythondir = $(ALTPYTHON_SITE_ARCH)

if ALTPYTHON_ENABLED
altpython_DATA = arc.py
altpython_LTLIBRARIES = alt/_arc.la
endif

if WIN32
AM_CPPFLAGS = -DWIN32 -DWINNT
endif

SWIG_INPUTS = $(top_srcdir)/swig/*.i

if PYDOXYGEN
PYDOXFLAGS = -DPYDOXYGEN
SWIG_INPUTS += pydoxygen.i
endif

ARCLIBS = \
	$(top_builddir)/src/hed/libs/credentialstore/libarccredentialstore.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la

_arc_la_SOURCES = arc_init.cpp
nodist__arc_la_SOURCES = arc_wrap.cpp
_arc_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(PYTHON_CFLAGS) \
	-fno-strict-aliasing -DSWIG_COBJECT_TYPES
_arc_la_LIBADD = \
	$(ARCLIBS) $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(PYTHON_LIBS)
_arc_la_LDFLAGS = -no-undefined -avoid-version -module

alt__arc_la_SOURCES = arc_init.cpp
nodist_alt__arc_la_SOURCES = arc_wrap.cpp
alt__arc_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(ALTPYTHON_CFLAGS) \
	-fno-strict-aliasing -DSWIG_COBJECT_TYPES
alt__arc_la_LIBADD = \
	$(ARCLIBS) $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(ALTPYTHON_LIBS)
alt__arc_la_LDFLAGS = -no-undefined -avoid-version -module

SWIG_OUTPUTS = arc_wrap.cpp arc.py

EXTRA_DIST = Doxyfile.api doxy2swig.py pydoxygen.i

CLEANFILES = $(SWIG_OUTPUTS) arc.pyc api

MAINTAINERCLEANFILES = pydoxygen.i

@AMDEP_TRUE@include ./$(DEPDIR)/arc_wrap.deps

arc.py: arc_wrap.cpp

arc_wrap.cpp: $(SWIG_INPUTS)
	mkdir -p $(DEPDIR)
	grep -h '^#' $(SWIG_INPUTS) | \
	$(CXXCOMPILE) $(_arc_la_CXXFLAGS) -M -MT arc_wrap.cpp -MT arc.py -MP -MF "$(DEPDIR)/arc_wrap.deps" -x c++ -
	$(SWIG) -v -c++ -python -threads -module arc -o arc_wrap.cpp \
		-I/usr/include -I$(top_srcdir)/include \
		$(PYDOXFLAGS) $(AM_CPPFLAGS) $(OPENSSL_CFLAGS) $(top_srcdir)/swig/Arc.i
	sed 's/\(^\s*char \*.*\) = \(.*ml_doc\)/\1 = (char *)\2/' arc_wrap.cpp > arc_wrap.cpp.new # Workaround for RHEL5 swig + EPEL5 python26
	mv arc_wrap.cpp.new arc_wrap.cpp
	sed 's/^\(\s*char \*cstr;\) int len;/#if PY_VERSION_HEX < 0x02050000 \&\& !defined(PY_SSIZE_T_MIN)\n&\n#else\n\1 Py_ssize_t len;\n#endif/' arc_wrap.cpp > arc_wrap.cpp.new # Ditto - for 64 bit
	mv arc_wrap.cpp.new arc_wrap.cpp
	sed '/*_wrap_delete_SwigPyIterator/,/SWIG_PYTHON_THREAD_END/ s/.*SWIG_PYTHON_THREAD_[A-Z]*_ALLOW.*//' arc_wrap.cpp > arc_wrap.cpp.new # Dont allow threading when deleting SwigPyIterator objects
	mv arc_wrap.cpp.new arc_wrap.cpp

pydoxygen.i: $(srcdir)/Doxyfile.api
	doxygen $(srcdir)/Doxyfile.api
	python $(srcdir)/doxy2swig.py api/xml/index.xml $@
	rm -rf api

install-data-hook:
	if test -n "$(PYTHON_SOABI)" ; then \
	  mv $(DESTDIR)$(pythondir)/_arc.so \
	  $(DESTDIR)$(pythondir)/_arc.$(PYTHON_SOABI).so ; \
	fi
	if test -n "$(ALTPYTHON_SOABI)" ; then \
	  mv $(DESTDIR)$(altpythondir)/_arc.so \
	  $(DESTDIR)$(altpythondir)/_arc.$(ALTPYTHON_SOABI).so ; \
	fi
