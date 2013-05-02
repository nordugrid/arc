SWIG_DEPENDS = \
	$(top_srcdir)/swig/common.i \
	$(top_srcdir)/swig/credential.i \
	$(top_srcdir)/swig/data.i \
	$(top_srcdir)/swig/compute.i

INPUT = $(top_srcdir)/src/hed/libs/common \
	$(top_srcdir)/src/hed/libs/credential \
	$(top_srcdir)/src/hed/libs/data \
	$(top_srcdir)/src/libs/data-staging \
	$(top_srcdir)/src/hed/libs/compute

CPP_EXAMPLES = $(top_srcdir)/src/hed/libs/common/examples \
	$(top_srcdir)/src/hed/libs/credential/examples \
	$(top_srcdir)/src/hed/libs/data/examples \
	$(top_srcdir)/src/libs/data-staging/examples \
	$(top_srcdir)/src/hed/libs/compute/examples
PYTHON_EXAMPLES = $(top_srcdir)/python/examples
JAVA_EXAMPLES = $(top_srcdir)/java/examples


Doxyfile.SDK.build: $(top_srcdir)/src/doxygen/Doxyfile.SDK
	cp $(srcdir)/Doxyfile.SDK Doxyfile.SDK.build
	sed "s/@TOP_SRCDIR@/$(subst /,\/,$(top_srcdir))/g" Doxyfile.SDK.build > Doxyfile.SDK.build.tmp
	mv Doxyfile.SDK.build.tmp Doxyfile.SDK.build
	sed "s/@INPUT@/$(subst /,\/,$(INPUT))/g" Doxyfile.SDK.build > Doxyfile.SDK.build.tmp
	mv Doxyfile.SDK.build.tmp Doxyfile.SDK.build
	sed "s/@EXAMPLES@/$(subst /,\/,$(CPP_EXAMPLES) $(PYTHON_EXAMPLES) $(JAVA_EXAMPLES))/g" Doxyfile.SDK.build > Doxyfile.SDK.build.tmp
	mv Doxyfile.SDK.build.tmp Doxyfile.SDK.build
	sed "s/Doxyfile.SDK.layout.xml/Doxyfile.SDK.build.layout.xml/g" Doxyfile.SDK.build > Doxyfile.SDK.build.tmp
	mv Doxyfile.SDK.build.tmp Doxyfile.SDK.build

Doxyfile.SDK.build.layout.xml: $(top_srcdir)/src/doxygen/Doxyfile.SDK.layout.xml
	cp $(srcdir)/Doxyfile.SDK.layout.xml Doxyfile.SDK.build.layout.xml

SDK: Doxyfile.SDK.build Doxyfile.SDK.build.layout.xml $(srcdir)/images/arcsdk.png $(SWIG_DEPENDS) $(wildcard $(addsuffix /*.h, $(INPUT))) $(wildcard $(addsuffix /*.cpp, $(CPP_EXAMPLES))) $(wildcard $(addsuffix /*.py, $(PYTHON_EXAMPLES))) $(wildcard $(addsuffix /*.java, $(JAVA_EXAMPLES)))
	doxygen Doxyfile.SDK.build
# Postprocessing: Add deviations from SDK API for language bindings (Python and Java).
	for file in $(SWIG_DEPENDS); do $(PYTHON) $(srcdir)/add-bindings-deviations-to-dox.py $${file} SDK/html; done

EXTRA_DIST = Doxyfile.SDK Doxyfile.SDK.layout.xml images/arcsdk.png

CLEANFILES = SDK
