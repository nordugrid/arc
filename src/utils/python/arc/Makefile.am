EXTRA_DIST = __init__.py gen_paths_dist.sh.in

pkgpythondir = $(PYTHON_SITE_ARCH)/arc
pkgpython_PYTHON = paths.py
nodist_pkgpython_PYTHON = paths_dist.py

paths_dist.py:
	$(SHELL) $(srcdir)/gen_paths_dist.sh > $@

SUBDIRS = utils control
DIST_SUBDIRS = utils control
