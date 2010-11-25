pythondir= $(PYTHON_SITE_PACKAGES)/storage
exampledir =  $(pkgdatadir)/examples

python_DATA = __init__.py client.py common.py
bin_SCRIPTS = chelonia
example_DATA = storage_service.xml.example
man_MANS = chelonia.1

EXTRA_DIST = $(python_DATA) $(example_DATA) $(bin_SCRIPTS)

if PYTHON_ENABLED
if WIN32
FUSE_CLIENT =
else
FUSE_CLIENT = fuse
endif
else
FUSE_CLIENT =
endif

SUBDIRS = librarian cli shepherd ahash bartender $(FUSE_CLIENT)
DIST_SUBDIRS = librarian cli shepherd ahash bartender fuse
