if ALTPYTHON_ENABLED
ALTPYTHON_DIR = altpython
else
ALTPYTHON_DIR =
endif

# Note that pydoxygen.i should really be made with PYTHON and ALTPYTHON
# For now we just do it with PYTHON since it is distributed
pydoxygen.i: $(top_srcdir)/python/Doxyfile.api.in
	sed "s/TOP_SOURCE_DIR/$(subst /,\/,$(top_srcdir))/g" $< > Doxyfile.api
	$(DOXYGEN) Doxyfile.api
	$(PYTHON) $(top_srcdir)/python/doxy2swig.py api/xml/index.xml $@
	rm -rf api Doxyfile.api

MAINTAINERCLEANFILES = pydoxygen.i

DIST_SUBDIRS = python altpython test examples
SUBDIRS = python $(ALTPYTHON_DIR) $(TEST_DIR) examples

EXTRA_DIST = Doxyfile.api.in doxy2swig.py __init__.py common.mk pydoxygen.i \
             swigmodulesinit_wrap.cpp arc_init.cpp


