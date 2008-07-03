pythondir= $(PYTHON_SITE_PACKAGES)/storage
docdir = @docdir@

python_DATA = __init__.py client.py common.py service.py xmltree.py
bin_SCRIPTS = standalone_storage_client.py
doc_DATA = storage.service.xml.example

EXTRA_DIST = $(python_DATA) $(doc_DATA) $(bin_SCRIPTS)

SUBDIRS = librarian cli shepherd ahash bartender
DIST_SUBDIRS = librarian cli shepherd ahash bartender

