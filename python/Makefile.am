if ALTPYTHON_ENABLED
ALTPYTHON_DIR = altpython
else
ALTPYTHON_DIR =
endif

DIST_SUBDIRS = python altpython test examples
SUBDIRS = python $(ALTPYTHON_DIR) $(TEST_DIR) examples

EXTRA_DIST = Doxyfile.api.in doxy2swig.py __init__.py common.mk \
             swigmodulesinit_wrap.cpp arc_init.cpp
