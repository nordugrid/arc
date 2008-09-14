pythondir= $(PYTHON_SITE_PACKAGES)/storage
docdir = @docdir@

python_DATA = __init__.py client.py common.py service.py xmltree.py
bin_SCRIPTS = arc_storage_cli
doc_DATA = storage.service.xml.example
man_MANS = arc_storage_cli.1

EXTRA_DIST = $(python_DATA) $(doc_DATA) $(bin_SCRIPTS)

SUBDIRS = librarian cli shepherd ahash bartender store
DIST_SUBDIRS = librarian cli shepherd ahash bartender store

