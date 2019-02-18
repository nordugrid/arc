pkglibexec_SCRIPTS = arcconfig-parser arccandypond
sbin_SCRIPTS = arcctl
pkgdata_DATA = arc.parser.defaults

man_MANS = arcctl.1

all-local:
	if test ! -e "$(abs_top_builddir)/python/python/arc/utils"; then ln -s $(abs_top_srcdir)/src/utils/python/arc/utils $(abs_top_builddir)/python/python/arc/utils; fi
	if test ! -e "$(abs_top_builddir)/python/python/arc/control"; then ln -s $(abs_top_srcdir)/src/utils/python/arc/control $(abs_top_builddir)/python/python/arc/control; fi

arc.parser.defaults: $(srcdir)/../../doc/arc.conf.reference
	$(PYTHON) $(srcdir)/arcconfig-reference --extract-defaults -r $< > $@

EXTRA_DIST = arcconfig-reference arccandypond

SUBDIRS = arc
DIST_SUBDIRS = arc

CLEANFILES = arc.parser.defaults \
	$(abs_top_builddir)/python/python/arc/utils \
	$(abs_top_builddir)/python/python/arc/control
