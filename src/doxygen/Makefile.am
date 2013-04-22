SWIG_DEPENDS = \
	$(top_srcdir)/swig/common.i \
	$(top_srcdir)/swig/credential.i \
	$(top_srcdir)/swig/data.i \
	$(top_srcdir)/swig/compute.i

Doxyfile.SDK.build: $(top_srcdir)/src/doxygen/Doxyfile.SDK
	cp $(srcdir)/Doxyfile.SDK Doxyfile.SDK.build
	sed "s/@TOP_SRCDIR@/$(subst /,\/,$(top_srcdir))/g" Doxyfile.SDK.build > Doxyfile.SDK.build.tmp
	mv Doxyfile.SDK.build.tmp Doxyfile.SDK.build
	sed "s/Doxyfile.SDK.layout.xml/Doxyfile.SDK.build.layout.xml/g" Doxyfile.SDK.build > Doxyfile.SDK.build.tmp
	mv Doxyfile.SDK.build.tmp Doxyfile.SDK.build

Doxyfile.SDK.build.layout.xml: $(top_srcdir)/src/doxygen/Doxyfile.SDK.layout.xml
	cp $(srcdir)/Doxyfile.SDK.layout.xml Doxyfile.SDK.build.layout.xml

# TODO: Add dependency on header files.
SDK: Doxyfile.SDK.build Doxyfile.SDK.build.layout.xml $(srcdir)/images/arcsdk.png $(SWIG_DEPENDS)
	doxygen Doxyfile.SDK.build
# Postprocessing: Add deviations from SDK API for language bindings (Python and Java).
	for file in $(SWIG_DEPENDS); do $(PYTHON) $(srcdir)/add-bindings-deviations-to-dox.py $${file} SDK/html; done

EXTRA_DIST = Doxyfile.SDK Doxyfile.SDK.layout.xml images/arcsdk.png

CLEANFILES = SDK
