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
	make -C services/latex
	mv services/latex/refman.pdf $@
	rm -rf services

ARC1-ChainComponents.pdf: Doxyfile.chain
	doxygen Doxyfile.chain
	make -C chain/latex
	mv chain/latex/refman.pdf $@
	rm -rf chain

EXTRA_DIST = Doxyfile.api Doxyfile.services Doxyfile.chain $(doc_DATA)

CLEANFILES = api services chain

MAINTAINERCLEANFILES = $(doc_DATA)
