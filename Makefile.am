ACLOCAL_FLAGS = -I m4 `test -d /opt/local/share/aclocal && echo -I /opt/local/share/aclocal`

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

SUBDIRS = src $(SWIG_SD) $(JAVA_SD) $(PYTHON_SD) intl doc

DIST_SUBDIRS = src swig java python intl doc include debian

EXTRA_DIST = arc.spec
