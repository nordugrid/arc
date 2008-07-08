docdir = @docdir@

doc_DATA = KnowARC-API.pdf KnowARC-Services.pdf KnowARC-ChainComponents.pdf

ngtest_PARTS = ngtest.tex
xrsl_PARTS = xrsl-urls.tex

KnowARC-API.pdf: Doxyfile.api
	doxygen Doxyfile.api && make -C latex && cp latex/refman.pdf KnowARC-API.pdf
	rm -rf latex

KnowARC-Services.pdf: Doxyfile.services
	doxygen Doxyfile.services && make -C latex && cp latex/refman.pdf KnowARC-Services.pdf
	rm -rf latex

KnowARC-ChainComponents.pdf: Doxyfile.chain
	doxygen Doxyfile.chain && make -C latex && cp latex/refman.pdf KnowARC-ChainComponents.pdf
	rm -rf latex


EXTRA_DIST = Doxyfile.api Doxyfile.services Doxyfile.chain hed KnowARC-API.pdf

CLEANFILES = latex

MAINTAINERCLEANFILES = $(doc_DATA)
