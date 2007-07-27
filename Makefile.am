ACLOCAL_FLAGS=-I m4

if JAVA_ENABLED 
JAVA_SD = java 
endif
if PYTHON_ENABLED
PYTHON_SD = python
endif

SUBDIRS = src swig $(JAVA_SD) $(PYTHON_SD) intl # doc
DIST_SUBDIRS = src swig java python intl doc

EXTRA_DIST = arc.spec
