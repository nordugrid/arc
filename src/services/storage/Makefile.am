pythondir= $(PYTHON_SITE_PACKAGES)/storage
docdir = @docdir@

python_DATA = __init__.py client.py common.py
bin_SCRIPTS = chelonia
doc_DATA = storage_service.xml.example
man_MANS = chelonia.1

EXTRA_DIST = $(python_DATA) $(doc_DATA) $(bin_SCRIPTS)

SUBDIRS = librarian cli shepherd ahash bartender
DIST_SUBDIRS = librarian cli shepherd ahash bartender

