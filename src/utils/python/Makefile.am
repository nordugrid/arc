dist_pkglibexec_SCRIPTS = arcconfig-parser
dist_sbin_SCRIPTS = arcctl

man_MANS = arcctl.1

all-local:
	if test ! -e "$(abs_top_builddir)/python/python/arc/utils"; then ln -s $(abs_top_srcdir)/src/utils/python/arc/utils $(abs_top_builddir)/python/python/arc/utils; fi
	if test ! -e "$(abs_top_builddir)/python/python/arc/control"; then ln -s $(abs_top_srcdir)/src/utils/python/arc/control $(abs_top_builddir)/python/python/arc/control; fi

SUBDIRS = arc
DIST_SUBDIRS = arc

CLEANFILES = $(abs_top_builddir)/python/python/arc/utils \
	$(abs_top_builddir)/python/python/arc/control
