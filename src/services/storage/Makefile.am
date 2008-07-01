pythondir= $(PYTHON_SITE_PACKAGES)/storage
docdir = @docdir@

python_DATA = __init__.py client.py common.py service.py xmltree.py
bin_SCRIPTS = standalone_manager_client.py
doc_DATA = storage.service.xml.example

EXTRA_DIST = $(python_DATA) $(doc_DATA) $(bin_SCRIPTS)

SUBDIRS = catalog cli element hash manager
DIST_SUBDIRS = catalog cli element hash manager

