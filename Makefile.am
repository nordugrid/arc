# /opt/local is the location for macports on MacOS X

ACLOCAL_AMFLAGS = -I m4 `test -d /opt/local/share/aclocal && echo -I /opt/local/share/aclocal`

distdir = nordugrid-arc1-@VERSION@

if JAVA_ENABLED 
JAVA_SD = java 
endif
if PYTHON_ENABLED
PYTHON_SD = python
endif
if SWIG_ENABLED
SWIG_SD = swig
endif
if DOC_ENABLED 
DOC_SD = doc
endif

SUBDIRS = src include $(SWIG_SD) $(JAVA_SD) $(PYTHON_SD) $(POSUB) $(DOC_SD)

DIST_SUBDIRS = src swig java python po doc include debian

EXTRA_DIST = arc.spec autogen.sh LICENSE
