docdir = @docdir@

doc_DATA = ARC1-API.pdf ARC1-Services.pdf ARC1-ChainComponents.pdf
noinst_DATA = pydoxygen.i

ARC1-API.pdf pydoxygen.i: Doxyfile.api
	doxygen Doxyfile.api && ( cd latex ; for f in *.tex ; do cp -p $$f $$f.old ; sed 's/\\doxyref{p\.}/\\doxyref{}{p\.}/g' $$f.old > $$f ; done ; make ) && cp latex/refman.pdf ARC1-API.pdf
	rm -rf latex
	python doxy2swig.py xml/index.xml pydoxygen.i
	rm -rf xml

ARC1-Services.pdf: Doxyfile.services
	doxygen Doxyfile.services && make -C latex && cp latex/refman.pdf $@
	rm -rf latex

ARC1-ChainComponents.pdf: Doxyfile.chain
	doxygen Doxyfile.chain && make -C latex && cp latex/refman.pdf $@
	rm -rf latex


EXTRA_DIST = Doxyfile.api Doxyfile.services Doxyfile.chain $(doc_DATA) doxy2swig.py

CLEANFILES = latex xml

MAINTAINERCLEANFILES = $(doc_DATA) $(noinst_DATA)
