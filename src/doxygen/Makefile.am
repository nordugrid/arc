docdir = @docdir@

doc_DATA = ARC1-API.pdf ARC1-Services.pdf ARC1-ChainComponents.pdf
noinst_DATA = pydoxygen.i

ARC1-API.pdf pydoxygen.i: Doxyfile.api
	doxygen Doxyfile.api
	( cd api/latex ; for f in *.tex ; do cp -p $$f $$f.old ; sed 's/\\doxyref{p\.}/\\doxyref{}{p\.}/g' $$f.old > $$f ; done ; make )
	mv api/latex/refman.pdf ARC1-API.pdf
	python doxy2swig.py api/xml/index.xml pydoxygen.i
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

EXTRA_DIST = Doxyfile.api Doxyfile.services Doxyfile.chain $(doc_DATA) doxy2swig.py

CLEANFILES = api services chain

MAINTAINERCLEANFILES = $(doc_DATA) $(noinst_DATA)
