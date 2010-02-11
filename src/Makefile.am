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
	test "x$(build_triple)" = "x$(host_triple)" && env LD_LIBRARY_PATH=$(DESTDIR)$(libdir):$(LD_LIBRARY_PATH) $(top_srcdir)/src/utils/arcplugin$(EXEEXT) -c $(DESTDIR)$(pkglibdir) || echo "No .apd files since we are cross-compiling"
endif
