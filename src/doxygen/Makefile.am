docdir = @docdir@

doc_DATA = ARC1-API.pdf ARC1-libarcdata.pdf

ARC1-API.pdf: $(srcdir)/Doxyfile.api
	doxygen $(srcdir)/Doxyfile.api
	( cd api/latex ; for f in *.tex ; do sed 's/\\doxyref{p\.}/\\doxyref{}{p\.}/g' $$f > $$f.new ; mv $$f.new $$f ; done )
	make -C api/latex
	mv api/latex/refman.pdf $@
	rm -rf api

ARC1-libarcdata.pdf: $(srcdir)/Doxyfile.data
	doxygen $(srcdir)/Doxyfile.data
	( cd data/latex ; for f in *.tex ; do sed 's/\\doxyref{p\.}/\\doxyref{}{p\.}/g' $$f > $$f.new ; mv $$f.new $$f ; done )
	make -C data/latex
	mv data/latex/refman.pdf $@
	rm -rf data

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

EXTRA_DIST = Doxyfile.api Doxyfile.data $(doc_DATA) Doxyfile.SDK Doxyfile.SDK.layout.xml images/arcsdk.png

CLEANFILES = api data SDK

MAINTAINERCLEANFILES = $(doc_DATA)

# This is alternative solution for doxygen+pdflatex problem. It is much slower
# but fixes directly on label names. Not sure which one is more correct. So
# keeping it for history
#	( cd services/latex; grep doxyref *.tex | sed 's/doxyref/\ndoxyref/g' | sed 's/^doxyref.*{p.}{\([^}]*\)}.*/\1/;t exit;s/.*//;:exit' | grep -F '_' | sort -u > badrefs ; for ref in `cat badrefs`; do nref=`echo $$ref | sed 's/_/-/g'` ; for file in *.tex; do cat $$file | sed "s/{$$ref}/{$$nref}/g" > $$file.new; mv -f $$file.new $$file; done; done )
