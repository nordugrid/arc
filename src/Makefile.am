if HED_ENABLED
if DOC_ENABLED
BUILD_SOURCES = doc hed libs tests services clients utils doxygen
else
BUILD_SOURCES = doc hed libs tests services clients utils
endif
else
BUILD_SOURCES =
endif

SUBDIRS = $(BUILD_SOURCES)
DIST_SUBDIRS = doc hed libs tests services clients utils doxygen

# This won't work in case of cross-compilation. Please
# some autotools experts fix it.
if HED_ENABLED
install-exec-hook:
	test "x$(build_triplet)" = "x$(host_triplet)" && env LD_LIBRARY_PATH=$(DESTDIR)$(libdir):$(LD_LIBRARY_PATH) $(top_builddir)/src/utils/hed/arcplugin$(EXEEXT) -c $(DESTDIR)$(pkglibdir) -c  $(DESTDIR)$(pkglibdir)/test || echo "No .apd files since we are cross-compiling"

uninstall-local:
	test "x$(build_triplet)" = "x$(host_triplet)" && rm -f $(DESTDIR)$(pkglibdir)/*.apd $(DESTDIR)$(pkglibdir)/test/*.apd
endif
