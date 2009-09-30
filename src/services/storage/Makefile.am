pythondir= $(PYTHON_SITE_PACKAGES)/storage
exampledir =  $(pkgdatadir)/examples

python_DATA = __init__.py client.py common.py
bin_SCRIPTS = chelonia
example_DATA = storage_service.xml.example
man_MANS = chelonia.1

EXTRA_DIST = $(python_DATA) $(example_DATA) $(bin_SCRIPTS)

SUBDIRS = librarian cli shepherd ahash bartender
DIST_SUBDIRS = librarian cli shepherd ahash bartender

