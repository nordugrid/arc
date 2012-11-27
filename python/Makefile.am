pythondir = $(PYTHON_SITE_ARCH)/arc

ARCSWIGLIBS = common loader message communication compute credential data delegation security
ARCPYLIBS = $(ARCSWIGLIBS:=.py)
ARCWRAPPERS = $(ARCSWIGLIBS:=_wrap.cpp)
ARCWRAPDEPS = $(foreach module, $(ARCSWIGLIBS), ./$(DEPDIR)/$(module)_wrap.deps)

if PYTHON_SWIG_ENABLED
python_DATA = __init__.py $(ARCPYLIBS)
python_LTLIBRARIES = _arc.la
endif


if ALTPYTHON_ENABLED
altpython_DATA = __init__.py $(ARCPYLIBS)
altpython_LTLIBRARIES = alt/_arc.la
altpythondir = $(ALTPYTHON_SITE_ARCH)/arc
else
altpythondir =
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
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/libs/data-staging/libarcdatastaging.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la

_arc_la_SOURCES = swigmodulesinit_wrap.cpp arc_init.cpp
nodist__arc_la_SOURCES = $(ARCWRAPPERS)
_arc_la_CXXFLAGS = -include ../config.h -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(PYTHON_CFLAGS) $(ZLIB_CFLAGS) \
	-fno-strict-aliasing -DSWIG_COBJECT_TYPES $(AM_CXXFLAGS)
_arc_la_LIBADD = \
	$(ARCLIBS) $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(PYTHON_LIBS) $(ZLIB_LIBS)
_arc_la_LDFLAGS = -no-undefined -avoid-version -module

alt__arc_la_SOURCES = swigmodulesinit_wrap.cpp arc_init.cpp
nodist_alt__arc_la_SOURCES = $(ARCWRAPPERS)
alt__arc_la_CXXFLAGS = -include ../config.h -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(ALTPYTHON_CFLAGS) $(ZLIB_CFLAGS) \
	-fno-strict-aliasing -DSWIG_COBJECT_TYPES $(AM_CXXFLAGS)
alt__arc_la_LIBADD = \
	$(ARCLIBS) $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(ALTPYTHON_LIBS) $(ZLIB_LIBS)
alt__arc_la_LDFLAGS = -no-undefined -avoid-version -module

SWIG_OUTPUTS = $(ARCWRAPPERS) $(ARCPYLIBS)

EXTRA_DIST = Doxyfile.api.in doxy2swig.py pydoxygen.i __init__.py

CLEANFILES = $(SWIG_OUTPUTS) api Doxyfile.api $(ARCPYLIBS:.py=.pyc)

MAINTAINERCLEANFILES = pydoxygen.i

@AMDEP_TRUE@-include $(ARCWRAPDEPS)

$(ARCPYLIBS): %.py: %_wrap.cpp

$(ARCWRAPPERS): %_wrap.cpp: $(top_srcdir)/swig/%.i pydoxygen.i
	mkdir -p $(DEPDIR)
	grep -h '^#' $(top_srcdir)/swig/$*.i | \
	$(CXXCOMPILE) $(_arc_la_CXXFLAGS) -M -MT $*_wrap.cpp -MT arc_$*.py -MP -MF "$(DEPDIR)/$*_wrap.deps" -x c++ -
	$(SWIG) -v -c++ -python -threads -o $*_wrap.cpp \
		-I/usr/include -I$(top_srcdir)/include \
		$(PYDOXFLAGS) $(AM_CPPFLAGS) $(OPENSSL_CFLAGS) $(top_srcdir)/swig/$*.i
# Workaround for RHEL5 swig + EPEL5 python26
	sed 's/\(^\s*char \*.*\) = \(.*ml_doc\)/\1 = (char *)\2/' $*_wrap.cpp > $*_wrap.cpp.new
	mv $*_wrap.cpp.new $*_wrap.cpp
# Ditto - for 64 bit
	sed 's/^\(\s*char \*cstr;\) int len;/#if PY_VERSION_HEX < 0x02050000 \&\& !defined(PY_SSIZE_T_MIN)\n&\n#else\n\1 Py_ssize_t len;\n#endif/' $*_wrap.cpp > $*_wrap.cpp.new
	mv $*_wrap.cpp.new $*_wrap.cpp
# Dont allow threading when deleting SwigPyIterator objects
	sed '/*_wrap_delete_@SWIG_PYTHON_NAMING@Iterator/,/SWIG_PYTHON_THREAD_END/ s/.*SWIG_PYTHON_THREAD_[A-Z]*_ALLOW.*//' $*_wrap.cpp > $*_wrap.cpp.new
	mv $*_wrap.cpp.new $*_wrap.cpp
# Dont allow threading when handling SWIG Python iterators (see bug
# 2683). Fixed in SWIG version 2.
	if test "x@SWIG2@" != "xyes"; then \
    sed '/*_wrap_@SWIG_PYTHON_NAMING@Iterator_/,/SWIG_PYTHON_THREAD_END/ s/.*SWIG_PYTHON_THREAD_[A-Z]*_ALLOW.*//' $*_wrap.cpp > $*_wrap.cpp.new; \
    mv $*_wrap.cpp.new $*_wrap.cpp; \
  fi
# When mapping a template with a template class no space is inserted
# between the two right angle brackets.
	sed 's/>>(new/> >(new/g' $*_wrap.cpp > $*_wrap.cpp.new
	mv $*_wrap.cpp.new $*_wrap.cpp
# When mapping a template with another template class as argument, and
# that template class takes two classes as argument, then older swigs
# put parentheses around the two class arguments, e.g. T<(A,B)>, not
# valid syntax should be T<A,B> instead.
	sed 's/<(\([,:[:alnum:]]*\))>/<\1>/g' $*_wrap.cpp > $*_wrap.cpp.tmp
	mv $*_wrap.cpp.tmp $*_wrap.cpp

pydoxygen.i: $(srcdir)/Doxyfile.api.in
	sed "s/TOP_SOURCE_DIR/$(subst /,\/,$(top_srcdir))/g" $< > Doxyfile.api
	doxygen Doxyfile.api
	python $(srcdir)/doxy2swig.py api/xml/index.xml $@
	rm -rf api Doxyfile.api

install-data-hook:
	if test -n "$(PYTHON_SOABI)" ; then \
	  mv $(DESTDIR)$(pythondir)/_arc.so \
	  $(DESTDIR)$(pythondir)/_arc.$(PYTHON_SOABI).so ; \
	fi
	if test -n "$(ALTPYTHON_SOABI)" ; then \
	  mv $(DESTDIR)$(altpythondir)/_arc.so \
	  $(DESTDIR)$(altpythondir)/_arc.$(ALTPYTHON_SOABI).so ; \
	fi

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)

.NOTPARALLEL: %.lo %.o
