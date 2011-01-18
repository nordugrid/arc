# /opt/local is the location for macports on MacOS X

ACLOCAL_AMFLAGS = -I m4 `test -d /opt/local/share/aclocal && echo -I /opt/local/share/aclocal`

distdir = nordugrid-arc-@VERSION@

if JAVA_ENABLED 
JAVA_SD = java 
endif
if PYTHON_ENABLED
PYTHON_SD = python
endif
if SWIG_ENABLED
SWIG_SD = swig
endif

SUBDIRS = src include $(SWIG_SD) $(JAVA_SD) $(PYTHON_SD) $(POSUB) debian nsis

DIST_SUBDIRS = src swig java python po include debian nsis

EXTRA_DIST = nordugrid-arc.spec mingw32-nordugrid-arc.spec nordugrid-arc.SlackBuild autogen.sh LICENSE

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = arcbase.pc
