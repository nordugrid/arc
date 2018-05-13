dist_pkglibexec_SCRIPTS = arcconfig-parser

man_MANS = arcctl.8

all-local:
	if test ! -e "$(abs_top_builddir)/python/python/arc/utils"; then ln -s $(abs_top_srcdir)/src/utils/configparser/arc/utils $(abs_top_builddir)/python/python/arc/utils; fi

SUBDIRS = arc
DIST_SUBDIRS = arc
