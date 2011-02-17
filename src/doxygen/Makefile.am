docdir = @docdir@

doc_DATA = ARC1-API.pdf ARC1-Services.pdf ARC1-ChainComponents.pdf

ARC1-API.pdf: Doxyfile.api
	doxygen Doxyfile.api
	( cd api/latex ; for f in *.tex ; do sed 's/\\doxyref{p\.}/\\doxyref{}{p\.}/g' $$f > $$f.new ; mv $$f.new $$f ; done )
	make -C api/latex
	mv api/latex/refman.pdf $@
	rm -rf api

ARC1-Services.pdf: Doxyfile.services
	doxygen Doxyfile.services
	( cd services/latex ; for f in *.tex ; do sed 's/\\doxyref{p\.}/\\doxyref{}{p\.}/g' $$f > $$f.new ; mv $$f.new $$f ; done )
	make -C services/latex
	mv services/latex/refman.pdf $@
	rm -rf services

ARC1-ChainComponents.pdf: Doxyfile.chain
	doxygen Doxyfile.chain
	( cd chain/latex ; for f in *.tex ; do sed 's/\\doxyref{p\.}/\\doxyref{}{p\.}/g' $$f > $$f.new ; mv $$f.new $$f ; done )
	make -C chain/latex
	mv chain/latex/refman.pdf $@
	rm -rf chain

EXTRA_DIST = Doxyfile.api Doxyfile.services Doxyfile.chain $(doc_DATA)

CLEANFILES = api services chain

MAINTAINERCLEANFILES = $(doc_DATA)

# This is alternative solution for doxygen+pdflatex problem. It is much slower
# but fixes directly on label names. Not sure which one is more correct. So
# keeping it for history
#	( cd services/latex; grep doxyref *.tex | sed 's/doxyref/\ndoxyref/g' | sed 's/^doxyref.*{p.}{\([^}]*\)}.*/\1/;t exit;s/.*//;:exit' | grep -F '_' | sort -u > badrefs ; for ref in `cat badrefs`; do nref=`echo $$ref | sed 's/_/-/g'` ; for file in *.tex; do cat $$file | sed "s/{$$ref}/{$$nref}/g" > $$file.new; mv -f $$file.new $$file; done; done )
