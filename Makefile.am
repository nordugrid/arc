# /opt/local is the location for macports on MacOS X

ACLOCAL_AMFLAGS = -I m4 `test -d /opt/local/share/aclocal && echo -I /opt/local/share/aclocal`

if JAVA_ENABLED 
JAVA_SD = java 
endif
if SWIG_ENABLED
SWIG_SD = swig
endif

SUBDIRS = src include $(SWIG_SD) $(JAVA_SD) python $(POSUB) debian nsis

DIST_SUBDIRS = src include swig java python po debian nsis

EXTRA_DIST = nordugrid-arc.spec mingw32-nordugrid-arc.spec \
	nordugrid-arc.SlackBuild autogen.sh LICENSE \
	selinux/nordugrid-arc-egiis.te selinux/nordugrid-arc-egiis.fc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = arcbase.pc
