pkgpythondir = $(PYTHON_SITE_ARCH)/arc
pyexecdir    = $(PYTHON_SITE_ARCH)

ARCSWIGLIBS = common loader message communication compute credential data delegation security
ARCPYLIBS   = $(ARCSWIGLIBS:=.py)
ARCWRAPPERS = $(ARCSWIGLIBS:=_wrap.cpp)
ARCWRAPHDRS = $(ARCSWIGLIBS:=_wrap.h)
ARCWRAPDEPS = $(foreach module, $(ARCSWIGLIBS), ./$(DEPDIR)/$(module)_wrap.deps)
ARCSWIGINIT = swigmodulesinit_wrap.cpp arc_init.cpp

BUILT_SOURCES = $(ARCSWIGINIT) __init__.py

$(ARCSWIGINIT) __init__.py: %: $(top_srcdir)/python/%
	cp $< $@

if PYTHON_SWIG_ENABLED
nodist_pkgpython_PYTHON   = __init__.py $(ARCPYLIBS)
pyexec_LTLIBRARIES = _arc.la
endif

if PYDOXYGEN
PYDOXFLAGS   = -DPYDOXYGEN
PYDOXFILE    = pydoxygen.i
else
PYDOXFLAGS   =
PYDOXFILE    =
endif

# Only remake index.xml if it is unavailable.
# It really depends on Doxyfile.api, but this file changes whenever
# configure is run, and we do not want to remake index.xml all
# the time since doxygen is currently not required to be present.
#index.xml: $(top_srcdir)/python/Doxyfile.api
index.xml:
	$(DOXYGEN) $(top_builddir)/python/Doxyfile.api
	cp -p api/xml/index.xml $@
	rm -rf api

pydoxygen.i: index.xml
	$(PYTHON) $(top_srcdir)/python/doxy2swig.py $< $@

EXTRA_DIST = index.xml
MAINTAINERCLEANFILES = index.xml

if SQLITEJSTORE_ENABLED
SWIG_IS_SQLITEJSTORE_ENABLED = -DSQLITEJSTORE_ENABLED
else
SWIG_IS_SQLITEJSTORE_ENABLED =
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

nodist__arc_la_SOURCES = $(ARCSWIGINIT) $(ARCWRAPPERS)

_arc_la_CXXFLAGS = -include $(top_builddir)/config.h \
        -I$(top_srcdir)/include -I$(top_builddir)/include \
        $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(OPENSSL_CFLAGS) $(PYTHON_CFLAGS) $(ZLIB_CFLAGS) $(SQLITE_CFLAGS) \
        -fno-strict-aliasing -DSWIG_COBJECT_TYPES -DPY_SSIZE_T_CLEAN $(AM_CXXFLAGS)
_arc_la_LIBADD = \
        $(ARCLIBS) $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(PYTHON_LIBS) $(ZLIB_LIBS) $(SQLITE_LIBS)
_arc_la_LDFLAGS = -no-undefined -avoid-version -module

CLEANFILES = $(ARCWRAPPERS) $(ARCWRAPHDRS) $(ARCPYLIBS) $(BUILT_SOURCES) pydoxygen.i $(ARCPYLIBS:.py=.pyc)

clean-local:
	-rm -rf __pycache__

@AMDEP_TRUE@-include $(ARCWRAPDEPS)

$(ARCPYLIBS): %.py: %_wrap.cpp

$(ARCWRAPPERS): %_wrap.cpp: $(top_srcdir)/swig/%.i $(top_srcdir)/swig/Arc.i $(PYDOXFILE)
	mkdir -p $(DEPDIR)
	grep -h '^#' $< $(top_srcdir)/swig/Arc.i $(PYDOXFILE) | \
	$(CXXCOMPILE) $(_arc_la_CXXFLAGS) -M -MT $*_wrap.cpp -MT arc_$*.py -MP -MF "$(DEPDIR)/$*_wrap.deps" -x c++ -
	$(SWIG) -v -c++ -python $(SWIG_PY3) -threads -o $*_wrap.cpp \
		-I/usr/include -I$(top_srcdir)/include -I$(top_builddir)/include \
		$(PYDOXFLAGS) $(SWIG_IS_SQLITEJSTORE_ENABLED) \
		$(AM_CPPFLAGS) $(OPENSSL_CFLAGS) $(top_srcdir)/swig/$*.i
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
# Fix python3 relative import problem
	sed "s/^\(\s*\)\(import _$*.*\)/\1from arc \2/" < $*.py > $*.py.new
	mv $*.py.new $*.py
# In swig 2.0.12 the erase method of the SharedBenchmarkMap type (swig template) uses a wrong name for the wrapped std::map::erase method.
	sed 's/std_map_Sl_std_string_Sc_double_Sc_std_less_Sl_std_string_Sg__Sc_std_allocator_Sl_std_pair_Sl_std_string_SS_const_Sc_double_Sg__Sg__Sg/std_map_Sl_std_string_Sc_double_Sg/g' $*_wrap.cpp > $*_wrap.cpp.tmp
	mv $*_wrap.cpp.tmp $*_wrap.cpp
# Fix python3 relative import problem if the module imports other submodules through %import(module= in the *.i files
	for i in $(ARCSWIGLIBS); do\
          if grep -q "^import $$i" $*.py; then \
            sed "s/import $$i/from arc &/" < $*.py > $*.py.new ;\
            mv $*.py.new $*.py; \
          fi;\
        done

install-data-hook:
	if [ "$(PYTHON_EXT_SUFFIX)" != ".so" ]; then \
	  mv $(DESTDIR)$(pyexecdir)/_arc.so \
	  $(DESTDIR)$(pyexecdir)/_arc$(PYTHON_EXT_SUFFIX) ; \
	fi

.NOTPARALLEL: %.lo %.o
