if HED_ENABLED
BUILD_SOURCES = hed tests services clients utils
else
BUILD_SOURCES =
endif

SUBDIRS = $(BUILD_SOURCES)
DIST_SUBDIRS = hed tests services clients utils

# This won't work in case of cross-compilation. Please
# some autotools experts fix it.
if HED_ENABLED
install-exec-hook:
	$(DESTDIR)$(bindir)/arcplugin$(EXEEXT) -c $(pkglibdir)
endif
