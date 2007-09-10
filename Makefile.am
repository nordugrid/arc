ACLOCAL_FLAGS=-I m4

if JAVA_ENABLED 
JAVA_SD = java 
endif
if PYTHON_ENABLED
PYTHON_SD = python
endif
if SWIG_ENABLED
SWIG_SD = swig
endif

SUBDIRS = src $(SWIG_SD) $(JAVA_SD) $(PYTHON_SD) src/hed/mcc src/libs/data/dmc intl doc

DIST_SUBDIRS = src swig java python src/hed/mcc src/libs/data/dmc intl doc include

EXTRA_DIST = arc.spec
