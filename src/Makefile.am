if HED_ENABLED
if DOC_ENABLED
BUILD_SOURCES = external doc hed libs tests services clients utils doxygen
else
BUILD_SOURCES = external doc hed libs tests services clients utils
endif
else
BUILD_SOURCES =
endif

SUBDIRS = $(BUILD_SOURCES)
DIST_SUBDIRS = external doc hed libs tests services clients utils doxygen

pkgdata_DATA = arc.parser.defaults

arc.parser.defaults: $(srcdir)/doc/arc.conf.reference
   #if test ! -e "$(abs_top_builddir)/python/python/arc/utils"; then ln -s $(abs_top_srcdir)/src/utils/python/arc/utils $(abs_top_builddir)/python/python/arc/utils; fi
   $(srcdir)/utils/python/arcconfig-reference --extract-defaults -d $@ -r $<

# This won't work in case of cross-compilation. Please
# some autotools experts fix it.
if HED_ENABLED
install-exec-hook:
	if test "x$(build_triplet)" = "x$(host_triplet)"; then env LD_LIBRARY_PATH=$(DESTDIR)$(libdir):$(LD_LIBRARY_PATH) $(top_builddir)/src/utils/hed/arcplugin$(EXEEXT) -c $(DESTDIR)$(pkglibdir) -c  $(DESTDIR)$(pkglibdir)/test; else echo "No .apd files since we are cross-compiling"; fi

uninstall-local:
	test "x$(build_triplet)" = "x$(host_triplet)" && rm -f $(DESTDIR)$(pkglibdir)/*.apd $(DESTDIR)$(pkglibdir)/test/*.apd
endif
